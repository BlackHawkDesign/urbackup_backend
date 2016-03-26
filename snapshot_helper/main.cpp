#include <string>
#include <iostream>
#include <vector>
#include "../stringtools.h"
#include "../urbackupcommon/os_functions.h"
#include <stdlib.h>
#ifndef _WIN32
#include <unistd.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>
extern char **environ;
#endif

#define DEF_Server
#include "../Server.h"


CServer *Server;

#ifdef _WIN32
#include <Windows.h>

bool CopyFolder(std::wstring src, std::wstring dst)
{
	if(!os_create_dir(dst))
		return false;

	std::vector<SFile> curr_files=getFiles(src);
	for(size_t i=0;i<curr_files.size();++i)
	{
		if(curr_files[i].isdir)
		{
			bool b=CopyFolder(src+os_file_sep()+curr_files[i].name, dst+os_file_sep()+curr_files[i].name);
			if(!b)
				return false;
		}
		else
		{
			if(!os_create_hardlink(dst+os_file_sep()+curr_files[i].name, src+os_file_sep()+curr_files[i].name, false, NULL) )
			{
				BOOL b=CopyFileW( (src+os_file_sep()+curr_files[i].name).c_str(), (dst+os_file_sep()+curr_files[i].name).c_str(), FALSE);
				if(!b)
				{
					return false;
				}
			}
		}
	}

	return true;
}
#endif


std::string getBackupfolderPath(void)
{
	std::string fn;
#ifdef _WIN32
	fn=trim(getFile("backupfolder"));
#else
	fn=trim(getFile("/etc/urbackup/backupfolder"));
#endif
	if(fn.find("\n")!=std::string::npos)
		fn=getuntil("\n", fn);
	if(fn.find("\r")!=std::string::npos)
		fn=getuntil("\r", fn);
	
	return fn;
}

std::string handleFilename(std::string fn)
{
	fn=conv_filename(fn);
	if(fn=="..")
	{
		return "";
	}
	return fn;
}

#ifndef _WIN32
int exec_wait(const std::string& path, bool keep_stdout, ...)
{
	va_list vl;
	va_start(vl, keep_stdout);
	
	std::vector<char*> args;
	args.push_back(const_cast<char*>(path.c_str()));
	
	while(true)
	{
		const char* p = va_arg(vl, const char*);
		if(p==NULL) break;
		args.push_back(const_cast<char*>(p));
	}
	va_end(vl);
	
	args.push_back(NULL);
	
	pid_t child_pid = fork();
	
	if(child_pid==0)
	{
		environ = new char*[1];
		*environ=NULL;
		
		if(!keep_stdout)
		{
			int nullfd = open("/dev/null", O_WRONLY);
			
			if(nullfd!=-1)
			{
				if(dup2(nullfd, 1)==-1)
				{
					return -1;
				}
				
				if(dup2(nullfd, 2)==-1)
				{
					return -1;
				}
			}
			else
			{
				return -1;
			}
		}
		
		int rc = execvp(path.c_str(), args.data());
		exit(rc);
	}
	else
	{
		int status;
		waitpid(child_pid, &status, 0);
		if(WIFEXITED(status))
		{
			return WEXITSTATUS(status);
		}
		else
		{
			return -1;
		}
	}
}

bool chown_dir(const std::string& dir)
{
	passwd* user_info = getpwnam("urbackup");
	if(user_info)
	{
		int rc = chown(dir.c_str(), user_info->pw_uid, user_info->pw_gid);
		return rc!=-1;
	}
	return false;
}

std::string find_btrfs_cmd()
{
	static std::string btrfs_cmd;
	
	if(!btrfs_cmd.empty())
	{
		return btrfs_cmd;
	}
	
	if(exec_wait("btrfs", false, "--version", NULL)==0)
	{
		btrfs_cmd="btrfs";
		return btrfs_cmd;
	}
	else if(exec_wait("/sbin/btrfs", false, "--version", NULL)==0)
	{
		btrfs_cmd="/sbin/btrfs";
		return btrfs_cmd;
	}
	else if(exec_wait("/bin/btrfs", false, "--version", NULL)==0)
	{
		btrfs_cmd="/bin/btrfs";
		return btrfs_cmd;
	}
	else if(exec_wait("/usr/sbin/btrfs", false, "--version", NULL)==0)
	{
		btrfs_cmd="/usr/sbin/btrfs";
		return btrfs_cmd;
	}
	else if(exec_wait("/usr/bin/btrfs", false, "--version", NULL)==0)
	{
		btrfs_cmd="/usr/bin/btrfs";
		return btrfs_cmd;
	}
	else
	{
		btrfs_cmd="btrfs";
		return btrfs_cmd;
	}
}
#endif

bool create_subvolume(std::string subvolume_folder)
{
#ifdef _WIN32
	return os_create_dir(subvolume_folder);
#else
	int rc=exec_wait(find_btrfs_cmd(), true, "subvolume", "create", subvolume_folder.c_str(), NULL);
	chown_dir(subvolume_folder);
	return rc==0;
#endif
}

bool create_snapshot(std::string snapshot_src, std::string snapshot_dst)
{
#ifdef _WIN32
	return CopyFolder(widen(snapshot_src), widen(snapshot_dst));
#else
	int rc=exec_wait(find_btrfs_cmd(), true, "subvolume", "snapshot", snapshot_src.c_str(), snapshot_dst.c_str(), NULL);
	chown_dir(snapshot_dst);
	return rc==0;
#endif
}

bool remove_subvolume(std::string subvolume_folder)
{
#ifdef _WIN32
	return os_remove_nonempty_dir(widen(subvolume_folder));
#else
	int compat_rc = exec_wait(find_btrfs_cmd(), false, "subvolume", "delete", "-c", NULL);
	
	int rc;
	if(compat_rc==12)
	{
		rc=exec_wait(find_btrfs_cmd(), true, "subvolume", "delete", subvolume_folder.c_str(), NULL);
	}
	else
	{
		rc=exec_wait(find_btrfs_cmd(), true, "subvolume", "delete", "-c", subvolume_folder.c_str(), NULL);
	}
	return rc==0;
#endif
}	

bool is_subvolume(std::string subvolume_folder)
{
#ifdef _WIN32
	return true;
#else
	int rc=exec_wait(find_btrfs_cmd(), false, "subvolume", "list", subvolume_folder.c_str(), NULL);
	return rc==0;
#endif
}	

int main(int argc, char *argv[])
{
	if(argc<2)
	{
		std::cout << "Not enough parameters" << std::endl;
		return 1;
	}

	std::string cmd=argv[1];

	std::string backupfolder=getBackupfolderPath();

	if(backupfolder.empty())
	{
		std::cout << "Backupfolder not set" << std::endl;
		return 1;
	}
	
#ifndef _WIN32
	if(seteuid(0)!=0)
	{
		std::cout << "Cannot become root user" << std::endl;
		return 1;
	}
#endif

	if(cmd=="create")
	{
		if(argc<4)
		{
			std::cout << "Not enough parameters for create" << std::endl;
			return 1;
		}

		std::string clientname=handleFilename(argv[2]);
		std::string name=handleFilename(argv[3]);

		std::string subvolume_folder=backupfolder+os_file_sep()+clientname+os_file_sep()+name;
		
		return create_subvolume(subvolume_folder)?0:1;
	}
	else if(cmd=="snapshot")
	{
		if(argc<5)
		{
			std::cout << "Not enough parameters for snapshot" << std::endl;
			return 1;
		}

		std::string clientname=handleFilename(argv[2]);
		std::string src_name=handleFilename(argv[3]);
		std::string dst_name=handleFilename(argv[4]);

		std::string subvolume_src_folder=backupfolder+os_file_sep()+clientname+os_file_sep()+src_name;
		std::string subvolume_dst_folder=backupfolder+os_file_sep()+clientname+os_file_sep()+dst_name;

		return create_snapshot(subvolume_src_folder, subvolume_dst_folder)?0:1;
	}
	else if(cmd=="remove")
	{
		if(argc<4)
		{
			std::cout << "Not enough parameters for remove" << std::endl;
			return 1;
		}

		std::string clientname=handleFilename(argv[2]);
		std::string name=handleFilename(argv[3]);

		std::string subvolume_folder=backupfolder+os_file_sep()+clientname+os_file_sep()+name;
		
		return remove_subvolume(subvolume_folder)?0:1;
	}
	else if(cmd=="test")
	{
		std::string clientdir=backupfolder+os_file_sep()+"testA54hj5luZtlorr494";
		
		bool create_dir_rc=os_create_dir(clientdir);
		if(!create_dir_rc)
		{	
			remove_subvolume(clientdir+os_file_sep()+"A");
			remove_subvolume(clientdir+os_file_sep()+"B");
			os_remove_dir(clientdir);
		}
		create_dir_rc = create_dir_rc || os_create_dir(clientdir);
		if(create_dir_rc)
		{	
			if(!create_subvolume(clientdir+os_file_sep()+"A") )
			{
				std::cout << "TEST FAILED: Creating test subvolume failed" << std::endl;
				os_remove_dir(clientdir);
				return 1;
			}
			
			bool suc=true;

			if(!create_snapshot(clientdir+os_file_sep()+"A", clientdir+os_file_sep()+"B") )
			{
				std::cout << "TEST FAILED: Creating test snapshot failed" << std::endl;
				suc=false;
			}
			
			if(suc)
			{			
				writestring("test", clientdir+os_file_sep()+"A"+os_file_sep()+"test");
				
				if(!os_create_hardlink(clientdir+os_file_sep()+"B"+os_file_sep()+"test", clientdir+os_file_sep()+"A"+os_file_sep()+"test", true, NULL))
				{
					std::cout << "TEST FAILED: Creating cross sub-volume reflink failed. Need Linux kernel >= 3.6." << std::endl;
					suc=false;
				}
				else
				{
					if(getFile(clientdir+os_file_sep()+"B"+os_file_sep()+"test")!="test")
					{
						std::cout << "TEST FAILED: Cannot read reflinked file" << std::endl;
						suc=false;
					}
				}
			}

			if(!remove_subvolume(clientdir+os_file_sep()+"A") )
			{
				std::cout << "TEST FAILED: Removing subvolume A failed" << std::endl;
				suc=false;
			}

			if(!remove_subvolume(clientdir+os_file_sep()+"B") )
			{
				std::cout << "TEST FAILED: Removing subvolume B failed" << std::endl;
				suc=false;
			}

			if(!os_remove_dir(clientdir))
			{
				std::cout << "TEST FAILED: Removing test clientdir failed" << std::endl;
				return 1;
			}
			
			if(!suc)
			{
				return 1;
			}
		}
		else
		{
			std::cout << "TEST FAILED: Creating test clientdir \"" << clientdir << "\" failed" << std::endl;
			return 1;
		}
		std::cout << "TEST OK" << std::endl;
		return 0;
	}
	else if(cmd=="issubvolume")
	{
		if(argc<4)
		{
			std::cout << "Not enough parameters for issubvolume" << std::endl;
			return 1;
		}

		std::string clientname=handleFilename(argv[2]);
		std::string name=handleFilename(argv[3]);

		std::string subvolume_folder=backupfolder+os_file_sep()+clientname+os_file_sep()+name;
		
		return is_subvolume(subvolume_folder)?0:1;
	}
	else
	{
		std::cout << "Command not found" << std::endl;
		return 1;
	}
}

