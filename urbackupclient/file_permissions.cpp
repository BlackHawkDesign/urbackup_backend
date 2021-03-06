#include "file_permissions.h"
#include "../Interface/Server.h"
#include "../stringtools.h"

#ifdef _WIN32
#include <Windows.h>
#include <Sddl.h>
#include <Aclapi.h>
#else
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#ifdef _WIN32

const TCHAR * szSD = TEXT("D:")       // Discretionary ACL
        //TEXT("(D;OICI;GA;;;WD)")     // Deny access to 
                                     // built-in guests
        TEXT("(A;OICI;GA;;;SY)") // Deny access to 
                                     // to authenticated 
                                     // users
        TEXT("(A;OICI;GA;;;BA)");    // Allow full control 
                                     // to administrators

bool change_file_permissions_admin_only(const std::string& filename)
{
	PSECURITY_DESCRIPTOR pSDCNV = NULL;

     BOOL b=ConvertStringSecurityDescriptorToSecurityDescriptor(
                szSD,
                SDDL_REVISION_1,
				&pSDCNV,
                NULL);

	 if(!b)
	 {
		 Server->Log("Error creating security descriptor", LL_ERROR);
		 return false;
	 }

	SECURITY_DESCRIPTOR sd = {};
	 DWORD sd_size = sizeof(sd);
	 PACL pDACL = NULL;
	 DWORD dwDACLSize = 0;
	 PACL pSACL = NULL;
	 DWORD dwSACLSize = 0;
	 DWORD dwOwnerSIDSize = 0;
	 DWORD dwGroupSIDSize = 0;


	 if (! MakeAbsoluteSD(pSDCNV, &sd, &sd_size, 
           pDACL, &dwDACLSize, 
           pSACL, &dwSACLSize, 
           NULL, &dwOwnerSIDSize, 
           NULL, &dwGroupSIDSize) ) {
 
 
		  pDACL = (PACL) GlobalAlloc(GPTR, dwDACLSize);
		  pSACL = (PACL) GlobalAlloc(GPTR, dwSACLSize);
 
		  if (! MakeAbsoluteSD(pSDCNV, &sd, &sd_size, pDACL, &dwDACLSize, 
				   pSACL, &dwSACLSize, NULL, &dwOwnerSIDSize, 
				   NULL, &dwGroupSIDSize) ) {
			Server->Log("Error: MakeAbsoluteSD", LL_ERROR);
			LocalFree(pSDCNV);
			GlobalFree(pDACL);
			GlobalFree(pSACL);
			return false;
		  }
	 }


	 bool ret=true;

	 DWORD rc = SetNamedSecurityInfoA(const_cast<char*>(filename.c_str()), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION, NULL, NULL, pDACL, NULL);
	 if(rc!=ERROR_SUCCESS)
	 {
		 Server->Log("Error setting security information. rc: "+nconvert((int)rc), LL_ERROR);
		 ret=false;
	 }

	 GlobalFree(pDACL);
	 GlobalFree(pSACL);

	 LocalFree(pSDCNV);

	 return ret;
}

bool write_file_only_admin(const std::string& data, const std::string& fn)
{
	SECURITY_ATTRIBUTES  sa;      
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = FALSE;


     BOOL b=ConvertStringSecurityDescriptorToSecurityDescriptor(
                szSD,
                SDDL_REVISION_1,
				&(sa.lpSecurityDescriptor),
                NULL);

	 if(!b)
	 {
		 Server->Log("Error creating security descriptor", LL_ERROR);
		 return false;
	 }

	 HANDLE file = CreateFileA(fn.c_str(),
		 GENERIC_READ | GENERIC_WRITE, 0, &sa, CREATE_ALWAYS, 0, NULL);

	 if(file==INVALID_HANDLE_VALUE)
	 {
		 Server->Log("Error creating pw file", LL_ERROR);
		 LocalFree(sa.lpSecurityDescriptor);
		 return false;
	 }

	 DWORD written=0;
	 while(written<data.size())
	 {
		b = WriteFile(file, data.data()+written, static_cast<DWORD>(data.size())-written, &written, NULL);
		if(!b)
		{
			Server->Log("Error writing to pw file", LL_ERROR);
			CloseHandle(file);
			LocalFree(sa.lpSecurityDescriptor);
			return false;
		}
	 }

	 CloseHandle(file);
	 LocalFree(sa.lpSecurityDescriptor);
	 return true;
}

#else

bool write_file_only_admin(const std::string& data, const std::string& fn)
{
	int fd = open(fn.c_str(), O_WRONLY|O_CREAT, S_IRWXU);

	if(fd==-1)
	{
		Server->Log("Error opening pw file", LL_ERROR);
		return false;
	}

	ssize_t rc = write(fd, data.data(), data.size());

	if(rc<data.size())
	{
		Server->Log("Error writing to pw file", LL_ERROR);
		close(fd);
		return false;
	}
	
	close(fd);
	return true;
}

bool change_file_permissions_admin_only(const std::string& filename)
{
	if(chmod(filename.c_str(), S_IRWXU)!=0)
	{
		Server->Log("Error setting file permissions", LL_ERROR);
		return false;
	}
	return true;
}

#endif