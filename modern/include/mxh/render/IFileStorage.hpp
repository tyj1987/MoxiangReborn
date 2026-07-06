// mxh/render/IFileStorage.hpp
// 1:1 port of original 4DyuchiGRX_common/IFileStorage.h.
// DX11 backend uses these interfaces to load .pak textures/models.
#pragma once

#include <objbase.h>   // IUnknown, STDMETHOD, REFIID, etc.

#include "file_storage_typedef.hpp"
#include "render_typedef.hpp"

namespace mxh::gx {

interface I4DyuchiFileStorage : public IUnknown {
    virtual BOOL    __stdcall Initialize(std::uint32_t dwMaxFileNum, std::uint32_t dwMaxFileHandleNumAtSameTime,
                                        std::uint32_t dwMaxFileNameLen, FILE_ACCESS_METHOD accessMethod) = 0;
    virtual void*   __stdcall MapPackFile(char* szPackFileName) = 0;
    virtual void    __stdcall UnmapPackFile(void* pPackFileHandle) = 0;

    virtual std::uint32_t __stdcall GetFileNum(void* pPackFileHandle) = 0;
    virtual std::uint32_t __stdcall CreateFileInfoList(void* pPackFileHandle, FSFILE_ATOM_INFO** ppInfoList,
                                                       std::uint32_t dwMaxNum) = 0;
    virtual void    __stdcall DeleteFileInfoList(void* pPackFileHandle, FSFILE_ATOM_INFO* pInfoList) = 0;
    virtual BOOL    __stdcall IsExistInFileStorage(char* szFileName) = 0;

    virtual BOOL    __stdcall LockPackFile(void* pPackFileHandle, std::uint32_t dwFlag) = 0;
    virtual BOOL    __stdcall InsertFileToPackFile(void* pPackFileHandle, char* szFileName) = 0;
    virtual BOOL    __stdcall DeleteFileFromPackFile(char* szFileName) = 0;
    virtual BOOL    __stdcall UnlockPackFile(void* pPackFileHandle, LOAD_CALLBACK_FUNC pCallBackFunc) = 0;

    virtual BOOL    __stdcall ExtractFile(char* szFileName) = 0;
    virtual BOOL    __stdcall ExtractAllFiles() = 0;
    virtual std::uint32_t __stdcall ExtractAllFilesFromPackFile(void* pPackFileHandle) = 0;

    virtual void*   __stdcall FSOpenFile(char* szFileName, std::uint32_t dwAccessMode) = 0;
    virtual int     __stdcall FSScanf(void* pFP, char* szFormat, ...) = 0;
    virtual std::uint32_t __stdcall FSRead(void* pFP, void* pDest, std::uint32_t dwLen) = 0;
    virtual std::uint32_t __stdcall FSSeek(void* pFP, std::uint32_t dwOffset, FSFILE_SEEK seekBase) = 0;
    virtual BOOL    __stdcall FSCloseFile(void* pFP) = 0;

    virtual BOOL    __stdcall GetPackFileInfo(void* pPackFileHandle, FSPACK_FILE_INFO* pFileInfo) = 0;

    virtual BOOL    __stdcall BeginLogging(char* szFileName, std::uint32_t dwFlag) = 0;
    virtual BOOL    __stdcall EndLogging() = 0;
};

} // namespace mxh::gx