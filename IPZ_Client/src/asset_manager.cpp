﻿#include "asset_manager.h"

void AssetManager::addAsset(std::shared_ptr<Asset> asset)
{
    assets.insert({asset->path, asset});
    auto dir =  std::make_shared<Dir>();
    auto p = asset->path;
    dir->path = p.remove_filename();
    auto r = dirs.insert({dir->path, dir});

    //If directory doesnt exist add it to map
    if(r.second)
    {
        dir->assets.insert({asset->path, asset});
        addDirWatch(dir);
    }
    else // if it does add asset to existing one
    {
        auto eDir = dirs[dir->path];
        eDir->assets.insert({asset->path, asset});
    }
}

void AssetManager::addDirWatch(std::shared_ptr<Dir> dir)
{

    auto pathStr = dir->path.native();
    const WCHAR* dirPath = pathStr.c_str();

    dir->handle = CreateFileW(
        dirPath,
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ/*|FILE_SHARE_DELETE|*/|FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OVERLAPPED ,
        NULL);

    //the buffer may overflow we should handle this, windows is providing us with info about this
    ZeroMemory(dir->buffer, sizeof(dir->buffer));
    ZeroMemory(&dir->overlapped, sizeof(dir->overlapped));
    dir->overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    auto ret = ReadDirectoryChangesW(
             dir->handle,
             &dir->buffer[dir->bufferIndex],
             sizeof(dir->buffer[dir->bufferIndex]),
             FALSE,
             FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME,
             NULL,
             &dir->overlapped,
             NULL);
    if(!ret)
        std::cout<<L"ReadDirectoryChangesW failed with"<<GetLastError()<<"\n";

}

void AssetManager::checkForChanges()
{
    // TODO: error logging
    for(auto& [path, dir] : dirs)
    {
        if (!dir->overlapped.hEvent)
            continue;

        DWORD dwObj = WaitForSingleObject(dir->overlapped.hEvent, 0);
        if (dwObj != WAIT_OBJECT_0)
            continue;

        DWORD dwNumberbytes;
        GetOverlappedResult(dir->handle, &dir->overlapped, &dwNumberbytes, FALSE);

        FILE_NOTIFY_INFORMATION *pFileNotify = dir->buffer[dir->bufferIndex];
        dir->bufferIndex = !dir->bufferIndex;

        ReadDirectoryChangesW(
            dir->handle,
            &dir->buffer[dir->bufferIndex],
            sizeof(dir->buffer[dir->bufferIndex]),
            FALSE,
            FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME,
            NULL,
            &dir->overlapped,
            NULL);

        while(true) {
            std::wstring filename(pFileNotify->FileName, pFileNotify->FileNameLength / sizeof(WCHAR));
            auto fullPath = dir->path;
            fullPath.replace_filename(filename);
            if(dir->assets.count(fullPath)>0)
            {
                dir->assets[fullPath]->reloadScheduled = true;
                std::cout<<"Asset '"<<fullPath<<"' found and marked for reload\n";
            }

            if (!pFileNotify->NextEntryOffset)
                break;
            pFileNotify = (FILE_NOTIFY_INFORMATION *)((PBYTE)pFileNotify + pFileNotify->NextEntryOffset);
        }
    }
}

void AssetManager::tryReloadAssets()
{
    for(auto& asset : assets)
    {
        if(asset.second->reloadScheduled)
            asset.second->doReload();
    }
}
