/*************************************************************************
*    UrBackup - Client/Server backup system
*    Copyright (C) 2011-2016 Martin Raiber
*
*    This program is free software: you can redistribute it and/or modify
*    it under the terms of the GNU Affero General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU Affero General Public License for more details.
*
*    You should have received a copy of the GNU Affero General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include "../stringtools.h"
#include "../urbackupcommon/settingslist.h"
#include "../urbackupcommon/os_functions.h"
#include <stdlib.h>
#include <assert.h>
#ifndef CLIENT_ONLY

#include "server_settings.h"
#include "../Interface/Server.h"
#include "server.h"

std::map<ServerSettings*, bool> ServerSettings::g_settings;
IMutex *ServerSettings::g_mutex=NULL;
std::map<int, SSettingsCacheItem> ServerSettings::g_settings_cache;

//#define CLEAR_SETTINGS_CACHE

void ServerSettings::init_mutex(void)
{
	if(g_mutex==NULL)
		g_mutex=Server->createMutex();
}

void ServerSettings::destroy_mutex(void)
{
	if(g_mutex!=NULL)
	{
		Server->destroy(g_mutex);
	}
}

void ServerSettings::clear_cache()
{
	IScopedLock lock(g_mutex);

	for(std::map<int, SSettingsCacheItem>::iterator it=g_settings_cache.begin();
		it!=g_settings_cache.end();)
	{
		if(it->second.refcount==0)
		{
			std::map<int, SSettingsCacheItem>::iterator delit=it++;
			delete delit->second.settings;
			g_settings_cache.erase(delit);
		}
		else
		{
			Server->Log("Refcount for settings for clientid \""+convert(it->second.settings->clientid)+"\" is not 0. Not deleting.", LL_WARNING);
			++it;
		}
	}
}

ServerSettings::ServerSettings(IDatabase *db, int pClientid)
	: local_settings(NULL), clientid(pClientid), settings_default(NULL),
		settings_client(NULL), db(db)
{
	IScopedLock lock(g_mutex);
		
	g_settings[this]=true;

	std::map<int, SSettingsCacheItem>::iterator iter=g_settings_cache.find(clientid);
	if(iter!=g_settings_cache.end())
	{
		++iter->second.refcount;
		settings_cache=&iter->second;
		do_update=settings_cache->needs_update;
		return;
	}
	else
	{
		SSettings* settings=new SSettings();
		SSettingsCacheItem cache_item = { settings, 1 , true};
		std::map<int, SSettingsCacheItem>::iterator iter = g_settings_cache.insert(std::make_pair(clientid, cache_item)).first;
		settings_cache=&iter->second;
		update(false);
		do_update=false;
	}	
}

void ServerSettings::createSettingsReaders()
{
	if(settings_default==NULL)
	{
		settings_default=Server->createDBSettingsReader(db, "settings", "SELECT value FROM settings_db.settings WHERE key=? AND clientid=0");
		if(clientid!=-1)
		{
			settings_client=Server->createDBSettingsReader(db, "settings", "SELECT value FROM settings_db.settings WHERE key=? AND clientid="+convert(clientid));
		}
		else
		{
			settings_client=NULL;
		}
	}
}

ServerSettings::~ServerSettings(void)
{
	if(settings_default!=NULL)
	{
		Server->destroy(settings_default);
	}
	if(settings_client!=NULL)
	{
		Server->destroy(settings_client);
	}

	{
		IScopedLock lock(g_mutex);

		std::map<ServerSettings*, bool>::iterator it=g_settings.find(this);
		assert(it!=g_settings.end());
		g_settings.erase(it);

		
		--settings_cache->refcount;
#ifdef CLEAR_SETTINGS_CACHE
		if(settings_cache->refcount==0)
		{
			std::map<int, SSettingsCacheItem>::iterator iter=g_settings_cache.find(clientid);
			assert(iter!=g_settings_cache.end());
			delete iter->second.settings;
			g_settings_cache.erase(iter);
		}
#endif
	}

	delete local_settings;
}

void ServerSettings::updateAll(void)
{
	IScopedLock lock(g_mutex);

	for(std::map<int, SSettingsCacheItem>::iterator it=g_settings_cache.begin();
		it!=g_settings_cache.end();)
	{
		if(it->second.refcount==0)
		{
			std::map<int, SSettingsCacheItem>::iterator delit=it++;
			delete delit->second.settings;
			g_settings_cache.erase(delit);
		}
		else
		{
			it->second.needs_update=true;
			++it;
		}
	}

	for(std::map<ServerSettings*, bool>::iterator it=g_settings.begin();
		it!=g_settings.end(); ++it)
	{
		it->first->doUpdate();
	}
}

void ServerSettings::update(bool force_update)
{
	createSettingsReaders();

	IScopedLock lock(g_mutex);

	if(settings_cache->needs_update || force_update)
	{
		readSettingsDefault();
		if(settings_client!=NULL)
		{
			readSettingsClient();
		}
		settings_cache->needs_update=false;
	}

	if(local_settings!=NULL)
	{
		delete local_settings;
		local_settings=new SSettings(*settings_cache->settings);
	}
}

void ServerSettings::doUpdate(void)
{
	do_update=true;
}

void ServerSettings::updateInternal(bool* was_updated)
{
	if(do_update)
	{
		if(was_updated!=NULL)
			*was_updated=true;

		do_update=false;
		update(false);
	}
	else
	{
		if(was_updated!=NULL)
			*was_updated=false;
	}
}

SSettings *ServerSettings::getSettings(bool *was_updated)
{
	updateInternal(was_updated);

	if(local_settings==NULL)
	{
		IScopedLock lock(g_mutex);
		local_settings=new SSettings(*settings_cache->settings);
	}

	return local_settings;
}

void ServerSettings::readSettingsDefault(void)
{
	SSettings* settings=settings_cache->settings;
	settings->clientid=clientid;
	settings->image_file_format=settings_default->getValue("image_file_format", image_file_format_default);
	settings->update_freq_incr=settings_default->getValue("update_freq_incr", convert(5*60*60) );
	settings->update_freq_full=settings_default->getValue("update_freq_full", convert(30*24*60*60) );
	settings->update_freq_image_incr=settings_default->getValue("update_freq_image_incr", convert( 7*24*60*60) );
	if(getImageFileFormatInt(settings->image_file_format)==image_file_format_cowraw)
	{
		settings->update_freq_image_full=settings_default->getValue("update_freq_image_full", convert( 60*24*60*60) );
	}
	else
	{
		settings->update_freq_image_full=settings_default->getValue("update_freq_image_full", convert( -60*24*60*60) );
	}	
	settings->max_file_incr=settings_default->getValue("max_file_incr", 100);
	settings->min_file_incr=settings_default->getValue("min_file_incr", 40);
	settings->max_file_full=settings_default->getValue("max_file_full", 10);
	settings->min_file_full=settings_default->getValue("min_file_full", 2);
	settings->min_image_incr=settings_default->getValue("min_image_incr", 4);
	settings->max_image_incr=settings_default->getValue("max_image_incr", 30);
	settings->min_image_full=settings_default->getValue("min_image_full", 2);
	settings->max_image_full=settings_default->getValue("max_image_full", 5);
	settings->no_images=(settings_default->getValue("no_images", "false")=="true");
	settings->no_file_backups=(settings_default->getValue("no_file_backups", "false")=="true");
	settings->overwrite=false;
	settings->allow_overwrite=(settings_default->getValue("allow_overwrite", "true")=="true");
	settings->backupfolder=trim(settings_default->getValue("backupfolder", "C:\\urbackup"));
	settings->backupfolder_uncompr=trim(settings_default->getValue("backupfolder_uncompr", settings->backupfolder));
	settings->autoshutdown=(settings_default->getValue("autoshutdown", "false")=="true");;
	settings->startup_backup_delay=settings_default->getValue("startup_backup_delay", 0);
	settings->download_client=(settings_default->getValue("download_client", "true")=="true");
	settings->autoupdate_clients=(settings_default->getValue("autoupdate_clients", "true")=="true");
	settings->backup_window_incr_file=settings_default->getValue("backup_window_incr_file", "1-7/0-24");
	settings->backup_window_full_file=settings_default->getValue("backup_window_full_file", "1-7/0-24");
	settings->backup_window_incr_image=settings_default->getValue("backup_window_incr_image", "1-7/0-24");
	settings->backup_window_full_image=settings_default->getValue("backup_window_full_image", "1-7/0-24");
	settings->max_active_clients=settings_default->getValue("max_active_clients", 100);
	settings->max_sim_backups=settings_default->getValue("max_sim_backups", 10);
	settings->exclude_files=settings_default->getValue("exclude_files", "");
	settings->include_files=settings_default->getValue("include_files", "");
	settings->default_dirs=settings_default->getValue("default_dirs", "");
	settings->cleanup_window=settings_default->getValue("cleanup_window", "1-7/3-4");
	settings->allow_config_paths=(settings_default->getValue("allow_config_paths", "true")=="true");
	settings->allow_starting_full_file_backups=(settings_default->getValue("allow_starting_full_file_backups", "true")=="true");
	settings->allow_starting_incr_file_backups=(settings_default->getValue("allow_starting_incr_file_backups", "true")=="true");
	settings->allow_starting_full_image_backups=(settings_default->getValue("allow_starting_full_image_backups", "true")=="true");
	settings->allow_starting_incr_image_backups=(settings_default->getValue("allow_starting_incr_image_backups", "true")=="true");
	settings->allow_pause=(settings_default->getValue("allow_pause", "true")=="true");
	settings->allow_log_view=(settings_default->getValue("allow_log_view", "true")=="true");
	settings->allow_tray_exit=(settings_default->getValue("allow_tray_exit", "true")=="true");
	settings->image_letters=settings_default->getValue("image_letters", "C");
	settings->backup_database=(settings_default->getValue("backup_database", "true")=="true");
	settings->internet_server_port=(unsigned short)(atoi(settings_default->getValue("internet_server_port", "55415").c_str()));
	settings->client_set_settings=false;
	settings->internet_server=settings_default->getValue("internet_server", "");
	settings->internet_image_backups=(settings_default->getValue("internet_image_backups", "false")=="true");
	settings->internet_full_file_backups=(settings_default->getValue("internet_full_file_backups", "false")=="true");
	settings->internet_encrypt=(settings_default->getValue("internet_encrypt", "true")=="true");
	settings->internet_compress=(settings_default->getValue("internet_compress", "true")=="true");
	settings->internet_compression_level=atoi(settings_default->getValue("internet_compression_level", "6").c_str());
	settings->internet_speed=settings_default->getValue("internet_speed", "-1");
	settings->local_speed=settings_default->getValue("local_speed", "-1");
	settings->global_internet_speed=settings_default->getValue("global_internet_speed", "-1");
	settings->global_local_speed=settings_default->getValue("global_local_speed", "-1");
	settings->internet_mode_enabled=(settings_default->getValue("internet_mode_enabled", "false")=="true");
	settings->silent_update=(settings_default->getValue("silent_update", "false")=="true");
	settings->use_tmpfiles=(settings_default->getValue("use_tmpfiles", "false")=="true");
	settings->use_tmpfiles_images=(settings_default->getValue("use_tmpfiles_images", "false")=="true");
	settings->tmpdir=settings_default->getValue("tmpdir","");
	settings->local_full_file_transfer_mode=settings_default->getValue("local_full_file_transfer_mode", "hashed");
	settings->internet_full_file_transfer_mode=settings_default->getValue("internet_full_file_transfer_mode", "raw");
	settings->local_incr_file_transfer_mode=settings_default->getValue("local_incr_file_transfer_mode", "hashed");
	settings->internet_incr_file_transfer_mode=settings_default->getValue("internet_incr_file_transfer_mode", "blockhash");
	settings->local_image_transfer_mode=settings_default->getValue("local_image_transfer_mode", "hashed");
	settings->internet_image_transfer_mode=settings_default->getValue("internet_image_transfer_mode", "raw");
	settings->update_stats_cachesize=static_cast<size_t>(settings_default->getValue("update_stats_cachesize", 200*1024));
	settings->global_soft_fs_quota=settings_default->getValue("global_soft_fs_quota", "100%");
	settings->client_quota=settings_default->getValue("client_quota", "");
	settings->end_to_end_file_backup_verification=(settings_default->getValue("end_to_end_file_backup_verification", "false")=="true");
	settings->internet_calculate_filehashes_on_client=(settings_default->getValue("internet_calculate_filehashes_on_client", "true")=="true");
	settings->use_incremental_symlinks=(settings_default->getValue("use_incremental_symlinks", "true")=="true");
	settings->internet_connect_always=(settings_default->getValue("internet_connect_always", "false")=="true");
	settings->show_server_updates=(settings_default->getValue("show_server_updates", "true")=="true");
	settings->server_url=settings_default->getValue("server_url", "");
	settings->verify_using_client_hashes=(settings_default->getValue("verify_using_client_hashes", "false")=="true");
	settings->internet_readd_file_entries=(settings_default->getValue("internet_readd_file_entries", "true")=="true");
	settings->max_running_jobs_per_client=atoi(settings_default->getValue("max_running_jobs_per_client", "1").c_str());
	settings->create_linked_user_views=(settings_default->getValue("create_linked_user_views", "false")=="true");
	settings->background_backups=(settings_default->getValue("background_backups", "true")=="true");
	settings->local_incr_image_style=settings_default->getValue("local_incr_image_style", incr_image_style_to_full);
	settings->local_full_image_style=settings_default->getValue("local_full_image_style", full_image_style_full);
	settings->internet_incr_image_style=settings_default->getValue("internet_incr_image_style", incr_image_style_to_last);
	settings->internet_full_image_style=settings_default->getValue("internet_full_image_style", full_image_style_synthetic);
	settings->backup_ok_mod_file = settings_default->getValue("backup_ok_mod_file", 3.f);
	settings->backup_ok_mod_image = settings_default->getValue("backup_ok_mod_image", 3.f);
	settings->cbt_volumes = settings_default->getValue("cbt_volumes", "ALL");
	settings->cbt_crash_persistent_volumes = settings_default->getValue("cbt_crash_persistent_volumes", "-");
}

void ServerSettings::readSettingsClient(void)
{	
	SSettings* settings=settings_cache->settings;
	std::string stmp=settings_client->getValue("internet_authkey", std::string());
	if(!stmp.empty())
	{
		settings->internet_authkey=stmp;
	}
	else
	{
		settings->internet_authkey=generateRandomAuthKey();
	}

	settings->client_access_key = settings_client->getValue("client_access_key", std::string());

	readBoolClientSetting("overwrite", &settings->overwrite);

	if(settings->overwrite)
	{
		readBoolClientSetting("allow_overwrite", &settings->allow_overwrite);
	}

	if(!settings->overwrite && !settings->allow_overwrite)
		return;

	stmp=settings_client->getValue("update_freq_incr", std::string());
	if(!stmp.empty())
		settings->update_freq_incr=stmp;
	stmp=settings_client->getValue("update_freq_full", std::string());
	if(!stmp.empty())
		settings->update_freq_full=stmp;
	stmp=settings_client->getValue("update_freq_image_incr", std::string());
	if(!stmp.empty())
		settings->update_freq_image_incr=stmp;
	stmp=settings_client->getValue("update_freq_image_full", std::string());
	if(!stmp.empty())
		settings->update_freq_image_full=stmp;
	int tmp=settings_client->getValue("max_file_incr", -1);
	if(tmp!=-1)
		settings->max_file_incr=tmp;
	tmp=settings_client->getValue("min_file_incr", -1);
	if(tmp!=-1)
		settings->min_file_incr=tmp;
	tmp=settings_client->getValue("max_file_full", -1);
	if(tmp!=-1)
		settings->max_file_full=tmp;
	tmp=settings_client->getValue("min_file_full", -1);
	if(tmp!=-1)
		settings->min_file_full=tmp;
	tmp=settings_client->getValue("min_image_incr", -1);
	if(tmp!=-1)
		settings->min_image_incr=tmp;
	tmp=settings_client->getValue("max_image_incr", -1);
	if(tmp!=-1)
		settings->max_image_incr=tmp;
	tmp=settings_client->getValue("min_image_full", -1);
	if(tmp!=-1)
		settings->min_image_full=tmp;
	tmp=settings_client->getValue("max_image_full", -1);
	if(tmp!=-1)
		settings->max_image_full=tmp;
	tmp=settings_client->getValue("startup_backup_delay", -1);
	if(tmp!=-1)
		settings->startup_backup_delay=tmp;	
	std::string swtmp=settings_client->getValue("computername", "");
	if(!swtmp.empty())
		settings->computername=swtmp;
	if(settings_client->getValue("virtual_clients", &swtmp))
		settings->virtual_clients=swtmp;
	if(settings_client->getValue("exclude_files", &swtmp))
		settings->exclude_files=swtmp;
	if(settings_client->getValue("include_files", &swtmp))
		settings->include_files=swtmp;
	swtmp=settings_client->getValue("default_dirs", "");
	if(!swtmp.empty())
		settings->default_dirs=swtmp;
	stmp=settings_client->getValue("image_letters", "");
	if(!stmp.empty())
		settings->image_letters=stmp;
	stmp=settings_client->getValue("internet_speed", "");
	if(!stmp.empty())
		settings->internet_speed=stmp;
	stmp=settings_client->getValue("local_speed", "");
	if(!stmp.empty())
		settings->local_speed=stmp;

	readBoolClientSetting("client_set_settings", &settings->client_set_settings);
	readBoolClientSetting("internet_mode_enabled", &settings->internet_mode_enabled);
	readBoolClientSetting("internet_full_file_backups", &settings->internet_full_file_backups);
	readBoolClientSetting("internet_image_backups", &settings->internet_image_backups);
	readBoolClientSetting("internet_compress", &settings->internet_compress);
	readBoolClientSetting("internet_encrypt", &settings->internet_encrypt);
	readBoolClientSetting("internet_connect_always", &settings->internet_connect_always);

	if(!settings->overwrite)
		return;

	stmp=settings_client->getValue("backup_window_incr_file", "");
	if(!stmp.empty())
		settings->backup_window_incr_file=stmp;
	stmp=settings_client->getValue("backup_window_full_file", "");
	if(!stmp.empty())
		settings->backup_window_full_file=stmp;
	stmp=settings_client->getValue("backup_window_incr_image", "");
	if(!stmp.empty())
		settings->backup_window_incr_image=stmp;
	stmp=settings_client->getValue("backup_window_full_image", "");
	if(!stmp.empty())
		settings->backup_window_full_image=stmp;

	stmp=settings_client->getValue("client_quota", "");
	if(!stmp.empty())
		settings->client_quota=stmp;

	stmp=settings_client->getValue("image_file_format", "");
	if(!stmp.empty())
		settings->image_file_format=stmp;

	readStringClientSetting("local_incr_file_transfer_mode", &settings->local_incr_file_transfer_mode);
	readStringClientSetting("local_full_file_transfer_mode", &settings->local_full_file_transfer_mode);
	readStringClientSetting("internet_full_file_transfer_mode", &settings->internet_full_file_transfer_mode);
	readStringClientSetting("internet_incr_file_transfer_mode", &settings->internet_incr_file_transfer_mode);
	readStringClientSetting("local_image_transfer_mode", &settings->local_image_transfer_mode);
	readStringClientSetting("internet_image_transfer_mode", &settings->internet_image_transfer_mode);

	readBoolClientSetting("end_to_end_file_backup_verification", &settings->end_to_end_file_backup_verification);
	readBoolClientSetting("internet_calculate_filehashes_on_client", &settings->internet_calculate_filehashes_on_client);	
	readBoolClientSetting("silent_update", &settings->silent_update);

	readBoolClientSetting("allow_config_paths", &settings->allow_config_paths);
	readBoolClientSetting("allow_starting_full_file_backups", &settings->allow_starting_full_file_backups);
	readBoolClientSetting("allow_starting_incr_file_backups", &settings->allow_starting_incr_file_backups);
	readBoolClientSetting("allow_starting_full_image_backups", &settings->allow_starting_full_image_backups);
	readBoolClientSetting("allow_starting_incr_image_backups", &settings->allow_starting_incr_image_backups);
	readBoolClientSetting("allow_pause", &settings->allow_pause);
	readBoolClientSetting("allow_log_view", &settings->allow_log_view);
	readBoolClientSetting("allow_tray_exit", &settings->allow_tray_exit);
	readBoolClientSetting("verify_using_client_hashes", &settings->verify_using_client_hashes);
	readBoolClientSetting("internet_readd_file_entries", &settings->internet_readd_file_entries);
	readBoolClientSetting("background_backups", &settings->background_backups);
	readIntClientSetting("max_running_jobs_per_client", &settings->max_running_jobs_per_client);
	readBoolClientSetting("create_linked_user_views", &settings->create_linked_user_views);

	readStringClientSetting("local_incr_image_style", &settings->local_incr_image_style);
	readStringClientSetting("local_full_image_style", &settings->local_full_image_style);
	readStringClientSetting("internet_incr_image_style", &settings->internet_incr_image_style);
	readStringClientSetting("internet_full_image_style", &settings->internet_full_image_style);

	readStringClientSetting("cbt_volumes", &settings->cbt_volumes);
	readStringClientSetting("cbt_crash_persistent_volumes", &settings->cbt_crash_persistent_volumes);
}

void ServerSettings::readBoolClientSetting(const std::string &name, bool *output)
{
	std::string value;
	if(settings_client->getValue(name, &value) && !value.empty())
	{
		if(value=="true")
			*output=true;
		else if(value=="false")
			*output=false;
	}
}

void ServerSettings::readStringClientSetting(const std::string &name, std::string *output)
{
	std::string value;
	if(settings_client->getValue(name, &value) && !value.empty())
	{
		*output=value;
	}
}

void ServerSettings::readIntClientSetting(const std::string &name, int *output)
{
	std::string value;
	if(settings_client->getValue(name, &value) && !value.empty())
	{
		*output=atoi(value.c_str());
	}
}

void ServerSettings::readSizeClientSetting(const std::string &name, size_t *output)
{
	std::string value;
	if(settings_client->getValue(name, &value) && !value.empty())
	{
		*output=static_cast<size_t>(os_atoi64(value));
	}
}

std::vector<STimeSpan> ServerSettings::getCleanupWindow(void)
{
	std::string window=getSettings()->cleanup_window;
	return getWindow(window);
}

std::vector<STimeSpan> ServerSettings::getBackupWindowIncrFile(void)
{
	std::string window=getSettings()->backup_window_incr_file;
	return getWindow(window);
}

std::vector<STimeSpan> ServerSettings::getBackupWindowFullFile(void)
{
	std::string window=getSettings()->backup_window_full_file;
	return getWindow(window);
}

std::vector<STimeSpan> ServerSettings::getBackupWindowIncrImage(void)
{
	std::string window=getSettings()->backup_window_incr_image;
	return getWindow(window);
}

std::vector<STimeSpan> ServerSettings::getBackupWindowFullImage(void)
{
	std::string window=getSettings()->backup_window_full_image;
	return getWindow(window);
}

std::vector<std::string> ServerSettings::getBackupVolumes(const std::string& all_volumes, const std::string& all_nonusb_volumes)
{
	std::string vols=getSettings()->image_letters;
	if(strlower(vols)=="all")
	{
		vols=all_volumes;
	}
	else if(strlower(vols)=="all_nonusb")
	{
		vols=all_nonusb_volumes;
	}
	std::vector<std::string> ret;
	Tokenize(vols, ret, ";,");
	for(size_t i=0;i<ret.size();++i)
	{
		ret[i]=trim(ret[i]);
	}
	return ret;
}

std::vector<STimeSpan> ServerSettings::getWindow(std::string window)
{
	std::vector<std::string> toks;
	Tokenize(window, toks, ";");

	std::vector<STimeSpan> ret;

	for(size_t i=0;i<toks.size();++i)
	{
		std::vector<STimeSpan> add = parseTimeSpan(toks[i]);
		ret.insert(ret.end(), add.begin(), add.end());
	}

	return ret;
}

float ServerSettings::parseTimeDet(std::string t)
{
	if(t.find(":")!=std::string::npos)
	{
		std::string h=getuntil(":", t);
		std::string m=getafter(":", t);

		return (float)atoi(h.c_str())+(float)atoi(m.c_str())*(1.f/60.f);
	}
	else
	{
		return (float)atoi(t.c_str());
	}
}

int ServerSettings::parseDayOfWeek(std::string dow)
{
	if(dow.size()==1 && str_isnumber(dow[0])==true)
	{
		int r=atoi(dow.c_str());
		if(r==0) r=7;
		return r;
	}
	else
	{
		dow=strlower(dow);
		if(dow=="mon" || dow=="mo" ) return 1;
		if(dow=="tu" || dow=="tue" || dow=="tues" || dow=="di" ) return 2;
		if(dow=="wed" || dow=="mi" ) return 3;
		if(dow=="th" || dow=="thu" || dow=="thur" || dow=="thurs" || dow=="do" ) return 4;
		if(dow=="fri" || dow=="fr" ) return 5;
		if(dow=="sat" || dow=="sa" ) return 6;
		if(dow=="sun" || dow=="so" ) return 7;
		return -1;
	}
}

STimeSpan ServerSettings::parseTime(std::string t)
{
	if(t.find("-")!=std::string::npos)
	{
		std::string f=trim(getuntil("-", t));
		std::string b=trim(getafter("-", t));

		return STimeSpan(parseTimeDet(f), parseTimeDet(b) );
	}
	else
	{
		return STimeSpan();
	}
}

std::string ServerSettings::generateRandomAuthKey(size_t len)
{
	std::string rchars="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
	std::string key;
	std::vector<unsigned int> rnd_n=Server->getSecureRandomNumbers(len);
	for(size_t j=0;j<len;++j)
		key+=rchars[rnd_n[j]%rchars.size()];
	return key;
}

std::string ServerSettings::generateRandomBinaryKey(void)
{
	std::string key;
	key.resize(32);
	Server->secureRandomFill((char*)key.data(), 32);
	return key;
}

int ServerSettings::getUpdateFreqImageIncr()
{
	updateInternal(NULL);
	IScopedLock lock(g_mutex);
	return static_cast<int>(currentTimeSpanValue(settings_cache->settings->update_freq_image_incr)+1);
}

int ServerSettings::getUpdateFreqFileIncr()
{
	updateInternal(NULL);
	IScopedLock lock(g_mutex);
	return static_cast<int>(currentTimeSpanValue(settings_cache->settings->update_freq_incr)+1);
}

int ServerSettings::getUpdateFreqImageFull()
{
	updateInternal(NULL);
	IScopedLock lock(g_mutex);
	return static_cast<int>(currentTimeSpanValue(settings_cache->settings->update_freq_image_full)+1);
}

int ServerSettings::getUpdateFreqFileFull()
{
	updateInternal(NULL);
	IScopedLock lock(g_mutex);
	return static_cast<int>(currentTimeSpanValue(settings_cache->settings->update_freq_full)+1);
}

std::string ServerSettings::getImageFileFormat()
{
	std::string image_file_format = getSettings()->image_file_format;

	return getImageFileFormatInt(image_file_format);
}


std::string ServerSettings::getImageFileFormatInt( const std::string& image_file_format )
{
	if(image_file_format == image_file_format_default)
	{
		if(BackupServer::isSnapshotsEnabled())
		{
			return image_file_format_cowraw;
		}
		else
		{
			return image_file_format_default;
		}
	}
	else
	{
		return image_file_format;
	}
}


std::vector<STimeSpan> ServerSettings::parseTimeSpan(std::string time_span)
{
	std::vector<STimeSpan> ret;

	std::string c_tok=trim(time_span);
	std::string f_o=getuntil("/", c_tok);
	std::string b_o=getafter("/", c_tok);

	std::vector<std::string> dow_toks;
	Tokenize(f_o, dow_toks, ",");

	std::vector<std::string> time_toks;
	Tokenize(b_o, time_toks, ",");

	for(size_t l=0;l<dow_toks.size();++l)
	{
		f_o=trim(dow_toks[l]);
		if(f_o.find("-")!=std::string::npos)
		{
			std::string f1=trim(getuntil("-", f_o));
			std::string b2=trim(getafter("-", f_o));

			int start=parseDayOfWeek(f1);
			int stop=parseDayOfWeek(b2);

			if(stop<start) { int t=start; start=stop; stop=t; }

			if(start<1 || start>7 || stop<1 || stop>7)
			{
				ret.clear();
				return ret;
			}

			for(size_t o=0;o<time_toks.size();++o)
			{
				b_o=trim(time_toks[o]);

				STimeSpan ts=parseTime(b_o);
				if(ts.dayofweek==-1)
				{
					ret.clear();
					return ret;
				}

				ts.numdays = stop - start + 1;
				for(int j=start;j<=stop;++j)
				{				
					ts.dayofweek=j;
					ret.push_back(ts);
				}
			}
		}
		else
		{
			int j=parseDayOfWeek(f_o);
			if(j<1 || j>7)
			{
				ret.clear();
				return ret;
			}

			for(size_t o=0;o<time_toks.size();++o)
			{
				b_o=trim(time_toks[o]);
				STimeSpan ts=parseTime(b_o);
				if(ts.dayofweek==-1)
				{
					ret.clear();
					return ret;
				}
				ts.dayofweek=j;
				ret.push_back(ts);
			}
		}
	}

	return ret;
}

std::vector<std::pair<double, STimeSpan > > ServerSettings::parseTimeSpanValue(std::string time_span_value)
{
	std::vector<std::string> toks;

	Tokenize(time_span_value, toks, ";");

	std::vector<std::pair<double, STimeSpan > > ret;


	for(size_t i=0;i<toks.size();++i)
	{
		if(toks[i].find("@")==std::string::npos)
		{
			ret.push_back(std::make_pair(atof(toks[i].c_str()), STimeSpan()));
		}
		else
		{
			double val = atof(getuntil("@", toks[i]).c_str());
			
			std::vector<STimeSpan> timespans = parseTimeSpan(getafter("@", toks[i]));

			for(size_t i=0;i<timespans.size();++i)
			{
				ret.push_back(std::make_pair(val, timespans[i]));
			}
		}
	}

	return ret;
}

int ServerSettings::getLocalSpeed()
{
	return static_cast<int>(currentTimeSpanValue(getSettings()->local_speed)+0.5);
}

int ServerSettings::getGlobalLocalSpeed()
{
	return static_cast<int>(currentTimeSpanValue(getSettings()->global_local_speed)+0.5);
}

int ServerSettings::getInternetSpeed()
{
	return static_cast<int>(currentTimeSpanValue(getSettings()->internet_speed)+0.5);
}

int ServerSettings::getGlobalInternetSpeed()
{
	return static_cast<int>(currentTimeSpanValue(getSettings()->global_internet_speed)+0.5);
}

double ServerSettings::currentTimeSpanValue(std::string time_span_value)
{
	std::vector<std::pair<double, STimeSpan > > time_span_values = parseTimeSpanValue(time_span_value);

	double val = -1;
	double selected_time_span_duration=25.f;

	for(size_t i=0;i<time_span_values.size();++i)
	{
		std::vector<STimeSpan> single_time_span;
		single_time_span.push_back(time_span_values[i].second);
		if(time_span_values[i].second.duration()<selected_time_span_duration
			&& ( time_span_values[i].second.dayofweek==-1 
			     || isInTimeSpan(single_time_span) ) )
		{
			val = time_span_values[i].first;
			selected_time_span_duration = time_span_values[i].second.duration();
		}
	}

	return val;
}

namespace
{
	std::string remLeadingZeros(std::string t)
	{
		std::string r;
		bool in=false;
		for(size_t i=0;i<t.size();++i)
		{
			if(!in && t[i]!='0' )
				in=true;

			if(in)
			{
				r+=t[i];
			}
		}
		return r;
	}
}

bool ServerSettings::isInTimeSpan(std::vector<STimeSpan> bw)
{
	if(bw.empty()) return true;
	int dow=atoi(os_strftime("%w").c_str());
	if(dow==0) dow=7;

	float hm=(float)atoi(remLeadingZeros(os_strftime("%H")).c_str())+(float)atoi(remLeadingZeros(os_strftime("%M")).c_str())*(1.f/60.f);
	for(size_t i=0;i<bw.size();++i)
	{
		if(bw[i].dayofweek==dow)
		{
			if( (bw[i].start_hour<=bw[i].stop_hour && hm>=bw[i].start_hour && hm<=bw[i].stop_hour)
				|| (bw[i].start_hour>bw[i].stop_hour && (hm>=bw[i].start_hour || hm<=bw[i].stop_hour) ) )
			{
				return true;
			}
		}
	}

	return false;
}

SLDAPSettings ServerSettings::getLDAPSettings()
{
	createSettingsReaders();
	SLDAPSettings ldap_settings;
	ldap_settings.login_enabled = settings_default->getValue("ldap_login_enabled", "false")=="true";
	
	ldap_settings.server_name = settings_default->getValue("ldap_server_name", "example.com");
	ldap_settings.server_port = settings_default->getValue("ldap_server_port", 3268);
	ldap_settings.username_prefix = settings_default->getValue("ldap_username_prefix", "example\\");
	ldap_settings.username_suffix = settings_default->getValue("ldap_username_suffix", "");
	ldap_settings.group_class_query = settings_default->getValue("ldap_group_class_query", "DC=example,DC=com?memberOf,objectClass?sub?(sAMAccountName={USERNAME})");
	ldap_settings.group_key_name = settings_default->getValue("ldap_group_key_name", "memberOf");
	ldap_settings.class_key_name = settings_default->getValue("ldap_class_key_name", "objectClass");
	ldap_settings.group_rights_map = parseLdapMap(settings_default->getValue("ldap_group_rights_map", "CN=Domain Admins,*==>all=all"));
	ldap_settings.class_rights_map = parseLdapMap(settings_default->getValue("ldap_class_rights_map", "user==>lastacts={AUTOCLIENTS},progress={AUTOCLIENTS},status={AUTOCLIENTS},stop_backup={AUTOCLIENTS},start_backup=all,browse_backups=tokens"));

	return ldap_settings;
}

std::map<std::string, std::string> ServerSettings::parseLdapMap( const std::string& data )
{
	std::vector<std::string> mappings;
	std::map<std::string, std::string> ret;
	Tokenize(data, mappings, "/");
	for(size_t i=0;i<mappings.size();++i)
	{
		std::string source = getuntil("==>", data);
		std::string target = getafter("==>", data);
		ret[source] = target;
	}
	return ret;
}

std::string ServerSettings::ldapMapToString( const std::map<std::string, std::string>& ldap_map )
{
	std::string ret;
	for(std::map<std::string, std::string>::const_iterator it=ldap_map.begin();
		it!=ldap_map.end();++it)
	{
		if(!ret.empty())
		{
			ret+="/";
		}
		ret+=it->first+"==>"+it->second;
	}
	return ret;
}

#endif //CLIENT_ONLY

