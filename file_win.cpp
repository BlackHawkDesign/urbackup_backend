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

#include "Server.h"
#include "file.h"
#include "types.h"
#include "stringtools.h"
#include <sstream>

#ifdef MODE_WIN

size_t File::tmp_file_index = 0;
IMutex* File::index_mutex = NULL;
std::string File::random_prefix;

File::File()
	: hfile(INVALID_HANDLE_VALUE), is_sparse(false), more_extents(true), curr_extent(0), last_sparse_pos(0)
{

}

bool File::Open(std::string pfn, int mode)
{
	fn=pfn;
	DWORD dwCreationDisposition;
	DWORD dwDesiredAccess;
	DWORD dwShareMode=FILE_SHARE_READ;
	if( mode==MODE_READ
		|| mode==MODE_READ_DEVICE
		|| mode==MODE_READ_SEQUENTIAL
		|| mode==MODE_READ_SEQUENTIAL_BACKUP)
	{
		dwCreationDisposition=OPEN_EXISTING;
		dwDesiredAccess=GENERIC_READ;
	}
	else if( mode==MODE_WRITE )
	{
		DeleteFileInt(pfn);
		dwCreationDisposition=CREATE_NEW;
		dwDesiredAccess=GENERIC_WRITE;
	}
	else if( mode==MODE_APPEND )
	{
		dwCreationDisposition=OPEN_EXISTING;
		dwDesiredAccess=GENERIC_WRITE | GENERIC_READ;
	}
	else if( mode==MODE_TEMP )
	{
		dwCreationDisposition=CREATE_NEW;
		dwDesiredAccess=GENERIC_WRITE | GENERIC_READ;
	}
	else if( mode==MODE_RW 
		|| mode==MODE_RW_SEQUENTIAL
		|| mode==MODE_RW_CREATE
		|| mode==MODE_RW_READNONE
		|| mode== MODE_RW_DEVICE)
	{
		if(mode==MODE_RW
			|| mode==MODE_RW_SEQUENTIAL
			|| mode==MODE_RW_READNONE
			|| mode== MODE_RW_DEVICE)
		{
			dwCreationDisposition=OPEN_EXISTING;
		}
		else
		{
			dwCreationDisposition=OPEN_ALWAYS;
		}
		dwDesiredAccess=GENERIC_WRITE | GENERIC_READ;
	}

	if(mode==MODE_READ_DEVICE
		|| mode== MODE_RW_DEVICE)
	{
		dwShareMode|=FILE_SHARE_WRITE;
	}

	DWORD flags=FILE_ATTRIBUTE_NORMAL;
	if(mode==MODE_READ_SEQUENTIAL
		|| mode==MODE_READ_SEQUENTIAL_BACKUP
		|| mode==MODE_RW_SEQUENTIAL)
	{
		flags|=FILE_FLAG_SEQUENTIAL_SCAN;
	}
	if(mode==MODE_READ_SEQUENTIAL_BACKUP)
	{
		flags|=FILE_FLAG_BACKUP_SEMANTICS;
	}
	
	hfile=CreateFileW( Server->ConvertToWchar(fn).c_str(), dwDesiredAccess, dwShareMode, NULL, dwCreationDisposition, flags, NULL );

	if( hfile!=INVALID_HANDLE_VALUE )
	{
		if( mode==MODE_APPEND )
		{
			Seek( Size() );
		}
		return true;
	}
	else
	{
		DWORD err = GetLastError();
		hfile=NULL;
		return false;
	}
}

bool File::OpenTemporaryFile(const std::string &tmpdir, bool first_try)
{
	std::ostringstream filename;

	if(tmpdir.empty())
	{
		wchar_t tmpp[MAX_PATH];
		DWORD l;
		if((l=GetTempPathW(MAX_PATH, tmpp))==0 || l>MAX_PATH )
		{
			wcscpy_s(tmpp, L"C:\\");
		}
		
		filename << Server->ConvertFromWchar(tmpp);
	}
	else
	{
		filename << tmpdir;

		if(tmpdir[tmpdir.size()-1]!='\\')
		{
			filename << "\\";
		}
	}

	filename << "urb" << random_prefix << L"-" << std::hex;

	{
		IScopedLock lock(index_mutex);
		filename << ++tmp_file_index;
	}

	filename << ".tmp";

	if(!Open(filename.str(), MODE_TEMP))
	{
		if(first_try)
		{
			Server->Log("Creating temporary file at \"" + filename.str()+"\" failed. Creating directory \""+tmpdir+"\"...", LL_WARNING);
			BOOL b = CreateDirectoryW(Server->ConvertToWchar(tmpdir).c_str(), NULL);

			if(b)
			{
				return OpenTemporaryFile(tmpdir, false);
			}
			else
			{
				Server->Log("Creating directory \""+tmpdir+"\" failed.", LL_WARNING);
				return false;
			}
		}
		else
		{
			return false;
		}
	}
	else
	{
		return true;
	}
}

bool File::Open(void *handle, const std::string& pFilename)
{
	hfile=(HANDLE)handle;
	fn = pFilename;
	if( hfile!=INVALID_HANDLE_VALUE )
	{
		return true;
	}
	else
	{
		return false;
	}
}

std::string File::Read(_u32 tr, bool *has_error)
{
	std::string ret;
	ret.resize(tr);
	_u32 gc=Read((char*)ret.c_str(), tr, has_error);
	if( gc<tr )
		ret.resize( gc );
	
	return ret;
}

std::string File::Read(int64 spos, _u32 tr, bool *has_error)
{
	std::string ret;
	ret.resize(tr);
	_u32 gc=Read(spos, (char*)ret.c_str(), tr, has_error);
	if( gc<tr )
		ret.resize( gc );

	return ret;
}

_u32 File::Read(char* buffer, _u32 bsize, bool *has_error)
{
	DWORD read;
	BOOL b=ReadFile(hfile, buffer, bsize, &read, NULL );
	if(b==FALSE)
	{
#ifdef _DEBUG
		int err=GetLastError();
		Server->Log("Read error: "+convert(err));
#endif
		if(has_error)
		{
			*has_error=true;
		}
	}
	return (_u32)read;
}

_u32 File::Read(int64 spos, char* buffer, _u32 bsize, bool *has_error)
{
	OVERLAPPED overlapped = {};
	LARGE_INTEGER li;
	li.QuadPart = spos;
	overlapped.Offset = li.LowPart;
	overlapped.OffsetHigh = li.HighPart;

	DWORD read;
	BOOL b=ReadFile(hfile, buffer, bsize, &read, &overlapped );
	if(b==FALSE)
	{
#ifdef _DEBUG
		int err=GetLastError();
		Server->Log("Read error: "+convert(err));
#endif
		if (has_error)
		{
			*has_error = true;
		}
	}
	return (_u32)read;
}

_u32 File::Write(const std::string &tw, bool *has_error)
{
	return Write( tw.c_str(), (_u32)tw.size(), has_error );
}

_u32 File::Write(int64 spos, const std::string &tw, bool *has_error)
{
	return Write(spos, tw.c_str(), (_u32)tw.size(), has_error);
}

_u32 File::Write(const char* buffer, _u32 bsize, bool *has_error)
{
	DWORD written;
	if (WriteFile(hfile, buffer, bsize, &written, NULL) == FALSE)
	{
		if (has_error)
		{
			*has_error = true;
		}
	}
	return written;
}

_u32 File::Write(int64 spos, const char* buffer, _u32 bsize, bool *has_error)
{
	OVERLAPPED overlapped = {};
	LARGE_INTEGER li;
	li.QuadPart = spos;
	overlapped.Offset = li.LowPart;
	overlapped.OffsetHigh = li.HighPart;

	DWORD written;
	if (WriteFile(hfile, buffer, bsize, &written, &overlapped) == FALSE)
	{
		if (has_error)
		{
			*has_error = true;
		}
	}
	return written;
}

bool File::Seek(_i64 spos)
{
	LARGE_INTEGER tmp;
	tmp.QuadPart=spos;
	if( SetFilePointerEx(hfile, tmp, NULL, FILE_BEGIN) == FALSE )
	{
		int err=GetLastError();
		return false;
	}
	else
		return true;
}

_i64 File::Size(void)
{
	LARGE_INTEGER fs;
	GetFileSizeEx(hfile, &fs);

	return fs.QuadPart;
}

_i64 File::RealSize()
{
	return Size();
}

void File::Close()
{
	if( hfile!=NULL )
	{
		BOOL b=CloseHandle( hfile );
		hfile=NULL;
	}
}

void File::init_mutex()
{
	index_mutex = Server->createMutex();

	std::string rnd;
	rnd.resize(8);
	unsigned int timesec = static_cast<unsigned int>(Server->getTimeSeconds());
	memcpy(&rnd[0], &timesec, sizeof(timesec));
	Server->randomFill(&rnd[4], 4);

	random_prefix = bytesToHex(reinterpret_cast<unsigned char*>(&rnd[0]), rnd.size());
}

void File::destroy_mutex()
{
	Server->destroy(index_mutex);
}

bool File::setSparse()
{
	if (!is_sparse)
	{
		FILE_SET_SPARSE_BUFFER buf = { TRUE };
		DWORD ret_bytes;
		BOOL b = DeviceIoControl(hfile, FSCTL_SET_SPARSE, &buf,
			static_cast<DWORD>(sizeof(buf)), NULL, 0, &ret_bytes, NULL);

		if (!b)
		{
			return false;
		}

		is_sparse = true;
	}

	return true;
}

bool File::PunchHole( _i64 spos, _i64 size )
{
	if (!setSparse())
	{
		return false;
	}
	
	FILE_ZERO_DATA_INFORMATION zdi;

	zdi.FileOffset.QuadPart = spos;
	zdi.BeyondFinalZero.QuadPart = spos + size;

	DWORD ret_bytes;
	BOOL b = DeviceIoControl(hfile, FSCTL_SET_ZERO_DATA, &zdi,
		static_cast<DWORD>(sizeof(zdi)), NULL, 0, &ret_bytes, 0);

	if(!b)
	{
		return false;
	}
	else
	{
		return true;
	}
}

bool File::Sync()
{
	return FlushFileBuffers(hfile)!=0;
}

bool File::Resize(int64 new_size)
{
	int64 fsize = Size();

	if (new_size > fsize)
	{
		if (!setSparse())
		{
			return false;
		}
	}

	LARGE_INTEGER tmp;
	tmp.QuadPart = 0;
	LARGE_INTEGER curr_pos;
	if (SetFilePointerEx(hfile, tmp, &curr_pos, FILE_CURRENT) == FALSE)
	{
		return false;
	}

	tmp.QuadPart = new_size;
	if (SetFilePointerEx(hfile, tmp, NULL, FILE_BEGIN) == FALSE)
	{
		return false;
	}

	BOOL ret = SetEndOfFile(hfile);

	SetFilePointerEx(hfile, curr_pos, NULL, FILE_BEGIN);

	return ret == TRUE;
}

void File::resetSparseExtentIter()
{
	res_extent_buffer.clear();
	more_extents = true;
	curr_extent = 0;
}

IFsFile::SSparseExtent File::nextSparseExtent()
{
	while (!res_extent_buffer.empty()
		&& curr_extent<res_extent_buffer.size())
	{
		if (res_extent_buffer[curr_extent].FileOffset.QuadPart != last_sparse_pos)
		{
			IFsFile::SSparseExtent ret(last_sparse_pos, res_extent_buffer[curr_extent].FileOffset.QuadPart - last_sparse_pos);
			last_sparse_pos = res_extent_buffer[curr_extent].FileOffset.QuadPart + res_extent_buffer[curr_extent].Length.QuadPart;
			++curr_extent;
			return ret;
		}
		
		last_sparse_pos = res_extent_buffer[curr_extent].FileOffset.QuadPart + res_extent_buffer[curr_extent].Length.QuadPart;
		++curr_extent;
	}

	if (!more_extents)
	{
		int64 fsize = Size();
		if (last_sparse_pos!=-1 && last_sparse_pos != fsize)
		{
			IFsFile::SSparseExtent ret = IFsFile::SSparseExtent(last_sparse_pos, fsize - last_sparse_pos);
			last_sparse_pos = fsize;
			return ret;
		}

		return IFsFile::SSparseExtent();
	}

	int64 fsize = Size();

	FILE_ALLOCATED_RANGE_BUFFER query_range;
	query_range.FileOffset.QuadPart = last_sparse_pos;
	query_range.Length.QuadPart = fsize- last_sparse_pos;
	
	if (res_extent_buffer.empty())
	{
		res_extent_buffer.resize(10);
	}
	else
	{
		res_extent_buffer.resize(100);
	}

	DWORD output_bytes;
	BOOL b = DeviceIoControl(hfile, FSCTL_QUERY_ALLOCATED_RANGES,
		&query_range, sizeof(query_range), res_extent_buffer.data(), static_cast<DWORD>(res_extent_buffer.size()*sizeof(FILE_ALLOCATED_RANGE_BUFFER)),
		&output_bytes, NULL);

	more_extents = (!b && GetLastError() == ERROR_MORE_DATA);

	if (more_extents || b)
	{
		res_extent_buffer.resize(output_bytes / sizeof(FILE_ALLOCATED_RANGE_BUFFER));
		curr_extent = 0;
	}
	else
	{
		res_extent_buffer.clear();
		more_extents = false;
		curr_extent = 0;
		last_sparse_pos = -1;
	}

	return nextSparseExtent();
}

#endif
