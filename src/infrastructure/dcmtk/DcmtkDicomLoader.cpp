/**
 * @file DcmtkDicomLoader.cpp
 * @brief Implementation of DcmtkDicomLoader
 * @date 2026
 */

#include "DcmtkDicomLoader.h"

SDicomLoadResult DcmtkDicomLoader::load(const std::string &filePath)
{
    SDicomLoadResult result;
    auto [image, loadResult] = m_loader.loadFile(filePath);
    result.result = loadResult;
    result.errorMessage = CDicomLoader::errorMessage(loadResult);
    if (image)
    {
        result.image = std::shared_ptr<CDicomImage>(std::move(image));
    }
    return result;
}
