/*************************************************************************
*    UrBackup - Client/Server backup system
*    Copyright (C) 2011-2015 Martin Raiber
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

#include "action_header.h"
#include "../../Interface/File.h"
#include "../../urbackupcommon/os_functions.h"
#include "../../urbackupcommon/file_metadata.h"
#include "../../Interface/SettingsReader.h"
#include "../../cryptoplugin/ICryptoFactory.h"
#include "../server_settings.h"
#include "backups.h"
#include <memory>
#include <algorithm>
#include "../../fileservplugin/IFileServ.h"
#include "../server_status.h"
#include "../restore_client.h"

extern ICryptoFactory *crypto_fak;
extern IFileServ* fileserv;

namespace backupaccess
{
	std::string constructFilter(const std::vector<int> &clientid, std::string key)
	{
		std::string clientf="(";
		for(size_t i=0;i<clientid.size();++i)
		{
			clientf+=key+"="+nconvert(clientid[i]);
			if(i+1<clientid.size())
				clientf+=" OR ";
		}
		clientf+=")";
		return clientf;
	}
}


bool create_zip_to_output(const std::wstring& foldername, const std::wstring& hashfoldername, const std::wstring& filter, bool token_authentication,
	const std::vector<backupaccess::SToken> &backup_tokens, const std::vector<std::string> &tokens, bool skip_hashes);

namespace
{
	bool sendFile(Helper& helper, const std::wstring& filename)
	{
		THREAD_ID tid = Server->getThreadID();
		Server->setContentType(tid, "application/octet-stream");
		Server->addHeader(tid, "Content-Disposition: attachment; filename=\""+Server->ConvertToUTF8(ExtractFileName(filename))+"\"");
		IFile *in=Server->openFile(os_file_prefix(filename), MODE_READ);
		if(in!=NULL)
		{
			helper.releaseAll();

			Server->addHeader(tid, "Content-Length: "+nconvert(in->Size()) );
			char buf[4096];
			_u32 r;
			do
			{
				r=in->Read(buf, 4096);
				Server->WriteRaw(tid, buf, r, false);
			}
			while(r>0);
			Server->destroy(in);
			return true;
		}
		else
		{
			Server->Log(L"Error opening file \""+filename+L"\"", LL_ERROR);
			return false;
		}
	}

	bool sendZip(Helper& helper, const std::wstring& foldername, const std::wstring& hashfoldername, const std::wstring& filter, bool token_authentication,
		const std::vector<backupaccess::SToken>& backup_tokens, const std::vector<std::string>& tokens, bool skip_hashes)
	{
		std::wstring zipname=ExtractFileName(foldername)+L".zip";

		THREAD_ID tid = Server->getThreadID();
		Server->setContentType(tid, "application/octet-stream");
		Server->addHeader(tid, "Content-Disposition: attachment; filename=\""+Server->ConvertToUTF8(zipname)+"\"");
		helper.releaseAll();

		return create_zip_to_output(foldername, hashfoldername, filter, token_authentication,
			backup_tokens, tokens, skip_hashes);
	}

	std::vector<FileMetadata> getMetadata(std::wstring dir, const std::vector<SFile>& files, bool skip_special)
	{
		std::vector<FileMetadata> ret;
		ret.resize(files.size());

		if(dir.empty() || dir[dir.size()-1]!=os_file_sep()[0])
		{
			dir+=os_file_sep();
		}

		for(size_t i=0;i<files.size();++i)
		{
			if(skip_special && (files[i].name==L".hashes" || files[i].name==L"user_views") )
				continue;

			std::wstring metadata_fn;
			if(files[i].isdir)
				metadata_fn = dir+escape_metadata_fn(files[i].name)+os_file_sep()+metadata_dir_fn;
			else
				metadata_fn = dir+escape_metadata_fn(files[i].name);

			if(!read_metadata(metadata_fn, ret[i]) )
			{
				Server->Log(L"Error reading metadata of file "+dir+os_file_sep()+files[i].name, LL_ERROR);
			}
		}

		return ret;
	}

	FileMetadata getMetaData(std::wstring path, bool is_file)
	{
		if(path.empty() || (!is_file && path[path.size()-1]!=os_file_sep()[0]) )
		{
			path+=os_file_sep();
		}

		std::wstring metadata_fn;
		if(is_file)
		{
			metadata_fn = ExtractFilePath(path) + os_file_sep() + escape_metadata_fn(ExtractFileName(path));
		}
		else
		{
			metadata_fn = ExtractFilePath(path) + os_file_sep() + escape_metadata_fn(ExtractFileName(path))+os_file_sep()+metadata_dir_fn;
		}

		FileMetadata ret;
		if(!read_metadata(metadata_fn, ret) )
		{
			Server->Log(L"Error reading metadata of path "+path, LL_ERROR);
		}
		return ret;
	}

	int getClientid(IDatabase* db, const std::wstring& clientname)
	{
		IQuery* q=db->Prepare("SELECT id FROM clients WHERE name=?");
		q->Bind(clientname);
		db_results res=q->Read();
		q->Reset();

		if(!res.empty())
		{
			return watoi(res[0][L"id"]);
		}
		return -1;
	}

	std::wstring getClientname(IDatabase* db, int clientid)
	{
		IQuery *q=db->Prepare("SELECT name FROM clients WHERE id=?");
		q->Bind(clientid);
		db_results res=q->Read();
		q->Reset();
		if(!res.empty())
		{
			return res[0][L"name"];
		}
		else
		{
			return std::wstring();
		}
	}

	bool checkBackupTokens(const std::string& fileaccesstokens, const std::wstring& backupfolder, const std::wstring& clientname, const std::wstring& path)
	{
		std::vector<std::string> tokens;
		Tokenize(fileaccesstokens, tokens, ";");

		backupaccess::STokens backup_tokens = backupaccess::readTokens(backupfolder, clientname, path);
		if(backup_tokens.tokens.empty())
		{
			return false;
		}

		for(size_t i=0;i<backup_tokens.tokens.size();++i)
		{
			for(size_t j=0;j<tokens.size();++j)
			{
				if(backup_tokens.tokens[i].token.empty())
					continue;

				if(backup_tokens.tokens[i].token==tokens[j])
				{
					return true;
				}
			}
		}

		return false;
	}

	bool checkFileToken(const std::string& fileaccesstokens, const std::wstring& backupfolder, const std::wstring& clientname, const std::wstring& path, const std::wstring& filemetadatapath)
	{
		std::vector<std::string> tokens;
		Tokenize(fileaccesstokens, tokens, ";");

		backupaccess::STokens backup_tokens = backupaccess::readTokens(backupfolder, clientname, path);
		if(backup_tokens.tokens.empty())
		{
			return false;
		}

		FileMetadata metadata;
		if(!read_metadata(filemetadatapath, metadata))
		{
			return false;
		}

		return checkFileToken(backup_tokens.tokens, tokens, metadata);
	}
} //unnamed namespace

namespace backupaccess
{
	std::wstring getBackupFolder(IDatabase* db)
	{
		IQuery* q=db->Prepare("SELECT value FROM settings_db.settings WHERE key='backupfolder'");
		db_results res_bf=q->Read();
		q->Reset();
		if(!res_bf.empty() )
		{
			return res_bf[0][L"value"];
		}
		else
		{
			return std::wstring();
		}
	}

	std::string decryptTokens(IDatabase* db, const str_map& GET)
	{
		if(crypto_fak==NULL)
		{
			return std::string();
		}

		int clientid;
		str_map::const_iterator iter_clientname =GET.find(L"clientname");
		if(iter_clientname!=GET.end())
		{
			clientid = getClientid(db, iter_clientname->second);
		}
		else
		{
			str_map::const_iterator iter_clientid = GET.find(L"clientid");
			if(iter_clientid!=GET.end())
			{
				clientid = watoi(iter_clientid->second);
			}
			else
			{
				return std::string();
			}
		}

		if(clientid==-1)
		{
			return std::string();
		}

		ServerSettings server_settings(db, clientid);

		std::string client_key = server_settings.getSettings()->client_access_key;

		size_t i=0;
		str_map::const_iterator iter;
		do 
		{
			iter = GET.find(L"tokens"+convert(i));

			if(iter!=GET.end())
			{
				std::string decry = crypto_fak->decryptAuthenticatedAES(base64_decode_dash(wnarrow(iter->second)), client_key);
				if(!decry.empty())
				{
					return decry;
				}
			}
		} while (iter!=GET.end());

		return std::string();
	}

	STokens readTokens(const std::wstring& backupfolder, const std::wstring& clientname, const std::wstring& path)
	{
		if(backupfolder.empty() || clientname.empty() || path.empty())
		{
			return STokens();
		}

		std::auto_ptr<ISettingsReader> backup_tokens(Server->createFileSettingsReader(backupfolder+os_file_sep()+clientname+os_file_sep()+path+os_file_sep()+L".hashes"+os_file_sep()+L".urbackup_tokens.properties"));

		if(!backup_tokens.get())
		{
			return STokens();
		}

		std::string ids_str = backup_tokens->getValue("ids", "");
		std::vector<std::string> ids;
		Tokenize(ids_str, ids, ",");

		std::vector<SToken> ret;
		for(size_t i=0;i<ids.size();++i)
		{
			SToken token = { watoi64(widen(ids[i])),
				base64_decode_dash(backup_tokens->getValue(ids[i]+"."+"accountname", "")),
				backup_tokens->getValue(ids[i]+"."+"token", "") };

			ret.push_back(token);
		}
		STokens tokens = { backup_tokens->getValue("access_key", ""),
			ret };

		return tokens;
	}

	bool checkFileToken( const std::vector<SToken> &backup_tokens, const std::vector<std::string> &tokens, const FileMetadata &metadata )
	{
		bool has_permission=false;
		for(size_t i=0;i<backup_tokens.size();++i)
		{
			for(size_t j=0;j<tokens.size();++j)
			{
				if(backup_tokens[i].token.empty())
					continue;

				bool denied = false;
				if(backup_tokens[i].token==tokens[j] &&
					metadata.hasPermission(static_cast<int>(backup_tokens[i].id), denied))
				{
					has_permission=true;
				}
				else if(denied)
				{
					return false;
				}
			}
		}

		return has_permission;
	}

	JSON::Array get_backups_with_tokens(IDatabase * db, int t_clientid, std::wstring clientname, std::string* fileaccesstokens, int backupid_offset)
	{
		std::wstring backupfolder = getBackupFolder(db);

		Helper helper(Server->getThreadID(), NULL, NULL);

		IQuery *q=db->Prepare("SELECT id, strftime('"+helper.getTimeFormatString()+"', backuptime, 'localtime') AS t_backuptime, incremental, size_bytes, archived, archive_timeout, path FROM backups WHERE complete=1 AND done=1 AND clientid=? ORDER BY backuptime DESC");
		q->Bind(t_clientid);
		db_results res=q->Read();
		JSON::Array backups;
		for(size_t i=0;i<res.size();++i)
		{
			if(fileaccesstokens!=NULL && !checkBackupTokens(*fileaccesstokens, backupfolder, clientname, res[i][L"path"]) )
			{
				continue;
			}

			JSON::Object obj;
			obj.set("id", watoi(res[i][L"id"])+backupid_offset);
			obj.set("backuptime", res[i][L"t_backuptime"]);
			obj.set("incremental", watoi(res[i][L"incremental"]));
            obj.set("size_bytes", watoi64(res[i][L"size_bytes"]));
            int archived = watoi(res[i][L"archived"]);
            obj.set("archived", watoi(res[i][L"archived"]));
			_i64 archive_timeout=0;
			if(!res[i][L"archive_timeout"].empty())
			{
				archive_timeout=watoi64(res[i][L"archive_timeout"]);
				if(archive_timeout!=0)
				{
					archive_timeout-=Server->getTimeSeconds();
				}
			}
            if(archived!=0)
            {
                obj.set("archive_timeout", archive_timeout);
            }
			backups.add(obj);
		}

		return backups;
	}

	SPathInfo get_metadata_path_with_tokens(const std::wstring& u_path, std::string* fileaccesstokens, std::wstring clientname, std::wstring backupfolder, int* backupid, std::wstring backuppath)
	{
		SPathInfo ret;
		std::vector<std::string> tokens;
		if(fileaccesstokens)
		{
			ret.backup_tokens = readTokens(backupfolder, clientname, backuppath);							
			Tokenize(*fileaccesstokens, tokens, ";");
		}

		std::vector<std::wstring> t_path;
		Tokenize(u_path, t_path, L"/");

		for(size_t i=0;i<t_path.size();++i)
		{
			if(!t_path[i].empty() && t_path[i]!=L" " && t_path[i]!=L"." && t_path[i]!=L".."
				&& t_path[i].find(L"/")==std::string::npos
				&& t_path[i].find(L"\\")==std::string::npos )
			{
			
				ret.rel_path+=UnescapeSQLString(t_path[i]);
				ret.rel_metadata_path+=escape_metadata_fn(UnescapeSQLString(t_path[i]));
				
				std::wstring curr_full_dir = backupfolder+os_file_sep()+clientname+os_file_sep()+backuppath+(ret.rel_path.empty()?L"":(os_file_sep()+ret.rel_path));
				
				if(i==t_path.size()-1 && !os_directory_exists(os_file_prefix(curr_full_dir)) )
				{
					ret.is_file=true;
				}
				else
				{
					ret.rel_path+=os_file_sep();
					ret.rel_metadata_path+=os_file_sep();
				}

				if(fileaccesstokens)
				{
					std::wstring curr_metadata_dir=backupfolder+os_file_sep()+clientname+os_file_sep()+backuppath+os_file_sep()+L".hashes"+(ret.rel_metadata_path.empty()?L"":(os_file_sep()+ret.rel_metadata_path));

					if(!ret.is_file)
					{
						curr_metadata_dir+=metadata_dir_fn;
					}

					FileMetadata dir_metadata;
					if(!read_metadata(curr_metadata_dir, dir_metadata))
					{
						ret.can_access_path=false;
						break;
					}
					if(!checkFileToken(ret.backup_tokens.tokens, tokens, dir_metadata))
					{
						ret.can_access_path=false;
						break;
					}
				}
			}
		}

		ret.full_metadata_path = backupfolder+os_file_sep()+clientname+os_file_sep()+backuppath+os_file_sep()+L".hashes"+(ret.rel_metadata_path.empty()?L"":(os_file_sep()+ret.rel_metadata_path));
		ret.full_path = backupfolder+os_file_sep()+clientname+os_file_sep()+backuppath+(ret.rel_path.empty()?L"":(os_file_sep()+ret.rel_path));

		return ret;
	}

	std::wstring get_backup_path(IDatabase* db, int backupid, int t_clientid)
	{
		IQuery* q=db->Prepare("SELECT path FROM backups WHERE id=? AND clientid=?");
		q->Bind(backupid);
		q->Bind(t_clientid);
		db_results res=q->Read();
		q->Reset();
		if(!res.empty())
		{
			return res[0][L"path"];
		}
		else
		{
			return std::wstring();
		}
	}

	bool get_files_with_tokens(IDatabase* db, int* backupid, int t_clientid, std::wstring clientname, std::string* fileaccesstokens, const std::wstring& u_path, JSON::Object& ret)
	{
		Helper helper(Server->getThreadID(), NULL, NULL);

		db_results res;
		if(backupid)
		{
			IQuery* q=db->Prepare("SELECT path,strftime('"+helper.getTimeFormatString()+"', backuptime, 'localtime') AS backuptime FROM backups WHERE id=? AND clientid=?");
			q->Bind(*backupid);
			q->Bind(t_clientid);
			res=q->Read();
			q->Reset();

			if(!res.empty())
			{
				ret.set("backuptime", res[0][L"backuptime"]);
				ret.set("backupid", *backupid);
			}
		}
		else
		{
			IQuery* q=db->Prepare("SELECT id, path,strftime('"+helper.getTimeFormatString()+"', backuptime, 'localtime') AS backuptime FROM backups WHERE clientid=? ORDER BY backuptime DESC");
			q->Bind(t_clientid);
			res=q->Read();
			q->Reset();
		}

		std::wstring backupfolder=getBackupFolder(db);

		if(backupfolder.empty())
		{
			return false;
		}

		std::wstring path;
		std::vector<std::wstring> t_path;
		Tokenize(u_path, t_path, L"/");

		bool is_file=false;

		JSON::Array ret_files;
		for(size_t k=0;k<res.size();++k)
		{
			std::wstring backuppath=res[k][L"path"];

			if( !fileaccesstokens || checkBackupTokens(*fileaccesstokens, backupfolder, clientname, backuppath) )
			{
				SPathInfo path_info = get_metadata_path_with_tokens(u_path, fileaccesstokens, clientname, backupfolder, backupid, backuppath);

				if(fileaccesstokens && !path_info.can_access_path)
				{
					continue;
				}

				is_file = path_info.is_file;

				std::vector<std::string> tokens;
				if(fileaccesstokens)
				{	
					Tokenize(*fileaccesstokens, tokens, ";");
				}

				ret.set("clientname", clientname);
				ret.set("clientid", t_clientid);							
				ret.set("path", u_path);

				if(backupid)
				{
					std::vector<SFile> tfiles=getFiles(os_file_prefix(path_info.full_path), NULL);
					std::vector<FileMetadata> tmetadata=getMetadata(path_info.full_metadata_path, tfiles, path.empty());

					JSON::Array files;
					for(size_t i=0;i<tfiles.size();++i)
					{
						if(tfiles[i].isdir)
						{
							if(path.empty() && (tfiles[i].name==L".hashes" || tfiles[i].name==L"user_views") )
								continue;

							if(fileaccesstokens && 
								!checkFileToken(path_info.backup_tokens.tokens, tokens, tmetadata[i]))
							{
								continue;
							}

							JSON::Object obj;
							obj.set("name", tfiles[i].name);
							obj.set("dir", tfiles[i].isdir);
							obj.set("size", tfiles[i].size);
							obj.set("mod", tmetadata[i].last_modified);
							obj.set("creat", tmetadata[i].created);
							files.add(obj);
						}
					}
					for(size_t i=0;i<tfiles.size();++i)
					{
						if(!tfiles[i].isdir)
						{
							if(path.empty() && tfiles[i].name==L".urbackup_tokens.properties")
								continue;

							if(fileaccesstokens && 
								!checkFileToken(path_info.backup_tokens.tokens, tokens, tmetadata[i]))
							{
								continue;
							}

							JSON::Object obj;
							obj.set("name", tfiles[i].name);
							obj.set("dir", tfiles[i].isdir);
							obj.set("size", tfiles[i].size);
							obj.set("mod", tmetadata[i].last_modified);
							obj.set("creat", tmetadata[i].created);
							if(!tmetadata[i].shahash.empty())
							{
								obj.set("shahash", base64_encode(reinterpret_cast<const unsigned char*>(tmetadata[i].shahash.c_str()), static_cast<unsigned int>(tmetadata[i].shahash.size())));
							}
							files.add(obj);
						}
					}

					ret.set("files", files);
				}
				else
				{
					std::auto_ptr<IFile> f;

					if(path_info.is_file)
					{
						f.reset(Server->openFile(os_file_prefix(path_info.full_path), MODE_READ));
					}

					if( (path_info.is_file && f.get()) || os_directory_exists(os_file_prefix(path_info.full_path)) )
					{
						FileMetadata metadata = getMetaData(path_info.full_metadata_path, path_info.is_file);

						JSON::Object obj;
						obj.set("name", ExtractFileName(path_info.full_path));
						if(path_info.is_file)
						{
							obj.set("size", f->Size());
							obj.set("dir", false);
						}
						else
						{
							obj.set("size", 0);
							obj.set("dir", true);
						}
						obj.set("mod", metadata.last_modified);
						obj.set("creat", metadata.created);
						obj.set("backupid", res[k][L"id"]);
						obj.set("backuptime", res[k][L"backuptime"]);
						if(!metadata.shahash.empty())
						{
							obj.set("shahash", base64_encode(reinterpret_cast<const unsigned char*>(metadata.shahash.c_str()), static_cast<unsigned int>(metadata.shahash.size())));
						}
						ret_files.add(obj);
					}								
				}
			}
		}

		if(!backupid)
		{
			ret.set("single_item", true);
			ret.set("is_file", is_file);
			ret.set("files", ret_files);
		}

		return true;
	}

} //namespace backupaccess

ACTION_IMPL(backups)
{
	Helper helper(tid, &GET, &PARAMS);
	JSON::Object ret;
	SUser *session=helper.getSession();

	bool has_tokens = GET.find(L"tokens0")!=GET.end();

	bool token_authentication=false;
	std::string fileaccesstokens;
	if( (session==NULL || session->id==SESSION_ID_TOKEN_AUTH) && has_tokens)
	{
		token_authentication=true;
		fileaccesstokens = backupaccess::decryptTokens(helper.getDatabase(), GET);

		if(fileaccesstokens.empty())
		{
			return;
		}
		else
		{
			std::wstring ses=helper.generateSession(L"anonymous");
			ret.set("session", ses);
			GET[L"ses"]=ses;
			helper.update(tid, &GET, &PARAMS);
			if(helper.getSession())
			{
				helper.getSession()->mStr[L"fileaccesstokens"]=widen(fileaccesstokens);
				helper.getSession()->id = SESSION_ID_TOKEN_AUTH;
			}
		}
	}
	else if(session!=NULL && session->id==SESSION_ID_TOKEN_AUTH)
	{
		fileaccesstokens = wnarrow(session->mStr[L"fileaccesstokens"]);
		if(!fileaccesstokens.empty())
		{
			token_authentication=true;
		}
		else
		{
			return;
		}
	}

	if(token_authentication)
	{
		ret.set("token_authentication", true);
	}

	std::wstring sa=GET[L"sa"];
	std::string rights=helper.getRights("browse_backups");
	std::string archive_rights=helper.getRights("manual_archive");
	std::vector<int> clientid;
	if(rights!="tokens")
	{
		clientid = helper.getRightIDs(rights);
	}
	std::vector<int> clientid_archive=helper.getRightIDs(archive_rights);
	if(clientid.size()==1 && sa.empty() )
	{
		sa=L"backups";
		GET[L"clientid"]=convert(clientid[0]);
	}
	if( (session!=NULL && rights!="none" ) || token_authentication)
	{
		IDatabase *db=helper.getDatabase();
		if(sa.empty())
		{
			std::string qstr = "SELECT id, name, strftime('"+helper.getTimeFormatString()+"', lastbackup, 'localtime') AS lastbackup FROM clients";
			if(token_authentication)
			{
				if(GET.find(L"clientname")==GET.end())
				{
					std::vector<std::string> tokens;
					Tokenize(fileaccesstokens, tokens, ";");
					IQuery* q_clients = db->Prepare("SELECT clientid FROM tokens_on_client WHERE token=?");
					for(size_t i=0;i<tokens.size();++i)
					{
						q_clients->Bind(tokens[i]);
						db_results res = q_clients->Read();
						q_clients->Reset();

						if(!res.empty())
						{
							int n_clientid = watoi(res[0][L"clientid"]);
							if(std::find(clientid.begin(), clientid.end(), n_clientid)==
								clientid.end())
							{
								clientid.push_back(n_clientid);
							}
						}
					}

					if(!clientid.empty())
					{
						qstr+=" WHERE "+backupaccess::constructFilter(clientid, "id");
					}
					else
					{
						sa=L"backups";
					}
				}
				else
				{
					sa=L"backups";
				}				
			}
			else
			{
				if(!clientid.empty())
				{
					qstr+=" WHERE "+backupaccess::constructFilter(clientid, "id");
				}
			}

			qstr+=" ORDER BY name";

			if(sa!=L"backups")
			{
				IQuery *q=db->Prepare(qstr);
				db_results res=q->Read();
				q->Reset();
				JSON::Array clients;
				for(size_t i=0;i<res.size();++i)
				{
					JSON::Object obj;
					obj.set("name", res[i][L"name"]);
					obj.set("id", watoi(res[i][L"id"]));
					obj.set("lastbackup", res[i][L"lastbackup"]);
					clients.add(obj);
				}
				ret.set("clients", clients);
			}			
		}
		if(sa==L"backups")
		{
			int t_clientid;
			std::wstring clientname;

			if(token_authentication && GET.find(L"clientid")==GET.end())
			{
				clientname=GET[L"clientname"];
				t_clientid = getClientid(helper.getDatabase(), clientname);
				if(t_clientid==-1)
				{
					ret.set("error", "2");
					helper.Write(ret.get(false));
					return;
				}
			}
			else
			{
				t_clientid=watoi(GET[L"clientid"]);

				clientname = getClientname(helper.getDatabase(), t_clientid);
			}

			bool r_ok=token_authentication?false:helper.hasRights(t_clientid, rights, clientid);
			bool archive_ok=token_authentication?false:helper.hasRights(t_clientid, archive_rights, clientid_archive);

			if(r_ok || token_authentication)
			{
				if(archive_ok)
				{
					if(GET.find(L"archive")!=GET.end())
					{
						IQuery *q=db->Prepare("UPDATE backups SET archived=1, archive_timeout=0 WHERE id=?");
						q->Bind(watoi(GET[L"archive"]));
						q->Write();
					}
					else if(GET.find(L"unarchive")!=GET.end())
					{
						IQuery *q=db->Prepare("UPDATE backups SET archived=0 WHERE id=?");
						q->Bind(watoi(GET[L"unarchive"]));
						q->Write();
					}
				}

				JSON::Array backups = backupaccess::get_backups_with_tokens(db, t_clientid, clientname,
					token_authentication ? &fileaccesstokens : NULL, 0);

				ret.set("backups", backups);
				ret.set("can_archive", archive_ok);

				ret.set("clientname", clientname);
				ret.set("clientid", t_clientid);
			}
			else
			{
				ret.set("error", 2);
			}
		}
		else if(sa==L"files" || sa==L"filesdl" || sa==L"zipdl" || sa==L"clientdl" )
		{
			int t_clientid;
			std::wstring clientname;
			if(token_authentication && GET.find(L"clientid")==GET.end())
			{
				clientname=GET[L"clientname"];
				t_clientid = getClientid(helper.getDatabase(), clientname);
				if(t_clientid==-1)
				{
					ret.set("error", "2");
					helper.Write(ret.get(false));
					return;
				}
			}
			else
			{
				t_clientid = watoi(GET[L"clientid"]);
				clientname = getClientname(helper.getDatabase(), t_clientid);
			}
			bool r_ok = token_authentication ? true : 
				helper.hasRights(t_clientid, rights, clientid);

			if(r_ok)
			{
				bool has_backupid=GET.find(L"backupid")!=GET.end();
				int backupid=0;
				if(has_backupid)
				{
					backupid=watoi(GET[L"backupid"]);
				}
				std::wstring u_path=UnescapeHTML(GET[L"path"]);

				if(!clientname.empty() )
				{
					if( (sa==L"filesdl" || sa==L"zipdl" || sa==L"clientdl") && has_backupid)
					{
						std::wstring backupfolder = backupaccess::getBackupFolder(db);
						std::wstring backuppath = backupaccess::get_backup_path(db, backupid, t_clientid);

						if(backupfolder.empty())
						{
							return;
						}

						backupaccess::SPathInfo path_info = backupaccess::get_metadata_path_with_tokens(u_path, token_authentication ? &fileaccesstokens : NULL,
							clientname, backupfolder, has_backupid ? &backupid : NULL, backuppath);

						if(token_authentication && !path_info.can_access_path)
						{
							return;
						}

						std::vector<std::string> tokens;
						if(token_authentication)
						{
							Tokenize(fileaccesstokens, tokens, ";");
						}

						if(sa==L"filesdl")
						{
							if(!token_authentication || checkFileToken(fileaccesstokens, backupfolder, clientname, backuppath, path_info.full_metadata_path))
							{
								sendFile(helper, path_info.full_path);
							}
							return;
						}
						else if(sa==L"zipdl")
						{
							sendZip(helper, path_info.full_path, path_info.full_metadata_path, GET[L"filter"], token_authentication, path_info.backup_tokens.tokens, tokens, path_info.rel_path.empty());
							return;
						}
						else if(sa==L"clientdl" && fileserv!=NULL)
						{
							if(ServerStatus::getStatus(clientname).comm_pipe==NULL)
							{
								ret.set("err", "client_not_online");
								helper.Write(ret.get(false));
								return;
							}

							if(!create_clientdl_thread(clientname, t_clientid, t_clientid, path_info.full_path, path_info.full_metadata_path, GET[L"filter"], token_authentication,
								path_info.backup_tokens.tokens, tokens, path_info.rel_path.empty(), path_info.rel_path))
							{
								ret.set("err", "internal_error");
								helper.Write(ret.get(false));
								return;
							}

							ret.set("ok", "true");
							helper.Write(ret.get(false));
							return;
						}
					}
					else
					{
						if(!backupaccess::get_files_with_tokens(db, has_backupid ? &backupid : NULL, t_clientid, clientname, token_authentication ? &fileaccesstokens : NULL,
								u_path, ret))
						{
							return;
						}

						ret.set("clientname", clientname);
						ret.set("clientid", t_clientid);							
						ret.set("path", u_path);
					}
				}
			}
		}
	}
	else
	{
		ret.set("error", 1);
	}

	helper.Write(ret.get(false));
}
