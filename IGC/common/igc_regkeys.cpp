/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

#include "igc_regkeys.hpp"
#if defined(IGC_DEBUG_VARIABLES)

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/CommandLine.h>
#include "common/LLVMWarningsPop.hpp"
#include "3d/common/iStdLib/File.h"
#include "secure_mem.h"
#include "secure_string.h"
#include "AdaptorCommon/customApi.hpp"

#if defined(_WIN64) || defined(_WIN32)
#include <devguid.h>  // for GUID_DEVCLASS_DISPLAY
#include <initguid.h> // for DEVPKEY_*
#include <Devpkey.h>  // for DEVPKEY_*
#include <SetupAPI.h>
#include <Shlwapi.h>
#include <algorithm>
#endif

#include <list>
#include <vector>
#include <string>
#include <utility>
#include <fstream>
#include <sstream>
#include <mutex>
#include "Probe/Assertion.h"

// path for IGC registry keys
#define IGC_REGISTRY_KEY "SOFTWARE\\INTEL\\IGFX\\IGC"

SRegKeysList g_RegKeyList;

#if defined(_WIN64) || defined(_WIN32)

DEVICE_INFO::DEVICE_INFO(DEVINST deviceInstance)
{
    deviceID = 0;
    revisionID = 0;
    pciBus = 0;
    pciDevice = 0;
    pciFunction = 0;
    get_device_property(deviceInstance, CM_DRP_DEVICEDESC);
    get_device_property(deviceInstance, CM_DRP_COMPATIBLEIDS);
    get_device_property(deviceInstance, CM_DRP_DRIVER);
    get_device_property(deviceInstance, CM_DRP_LOCATION_INFORMATION);
}

void DEVICE_INFO::get_device_property(DEVINST deviceInstace, DWORD property)
{
    wchar_t propertyData[MAX_PATH];
    DWORD   propertyDataLength = 0;

    // First get the size of the data
    CONFIGRET status = CM_Get_DevNode_Registry_PropertyW(
        deviceInstace,
        property,
        NULL,
        NULL,
        &propertyDataLength,
        0);

    // Then fetch the actual property data
    if(CR_BUFFER_SMALL == status)
    {
        status = CM_Get_DevNode_Registry_PropertyW(
            deviceInstace,
            property,
            NULL,
            &propertyData[0],
            &propertyDataLength,
            0);
    }

    if(CR_SUCCESS == status)
    {
        // Convert from UTF-16 (wide char) to UTF-8
        DWORD convertedStringSize = WideCharToMultiByte(CP_UTF8, 0, &propertyData[0], sizeof(propertyData), NULL, 0, NULL, NULL);
        std::string propertyString(convertedStringSize, 0);
        WideCharToMultiByte(CP_UTF8, 0, &propertyData[0], sizeof(propertyData), &propertyString[0], convertedStringSize, NULL, NULL);

        switch(property)
        {
        case CM_DRP_COMPATIBLEIDS:
            // PCI\VEN_8086&DEV_1912&REV_06
            sscanf_s(propertyString.c_str(), "PCI\\VEN_8086&DEV_%x&REV_%x", &deviceID, &revisionID);
            break;
        case CM_DRP_DEVICEDESC:
            // Intel(R) HD Graphics 530
            description = propertyString.c_str();
            break;
        case CM_DRP_DRIVER:
            // {4d36e968-e325-11ce-bfc1-08002be10318}\0001
            driverRegistryPath = propertyString.c_str();
            break;
        case CM_DRP_LOCATION_INFORMATION:
            // PCI bus 0, device 2, function 0
            sscanf_s(propertyString.c_str(), "PCI bus %u, device %u, function %u", &pciBus, &pciDevice, &pciFunction);
            break;
        }
    }
    else if(CR_NO_SUCH_VALUE == status)
    {
        IGC_ASSERT_MESSAGE(0, "No such property value");
    }
    else
    {
        IGC_ASSERT_MESSAGE(0, "Failed to get DevNode property");
    }
}

/********************************************************************************************/
/* Function: ConvertDoubleNullTermStringToVector                                            */
/*                                                                                          */
/* Converts a Unicode string with embedded null characters                                  */
/* into a vector of strings                                                                 */
/*                                                                                          */
/********************************************************************************************/
static void ConvertDoubleNullTermStringToVector(std::vector<wchar_t>& input_str, std::vector<std::wstring>& output_vec)
{
    // Remove terminating null character to avoid adding empty string at the end
    input_str.resize(input_str.size() - 1);

    std::wstring str;
    std::for_each(input_str.cbegin(), input_str.cend(), [&](const wchar_t& w)
        {
            if (w != L'\0')
            {
                str += w;
            }
            else
            {
                output_vec.push_back(str);
                str = L"";
            }
        });
}


/*********************************************************************************************/
/* GetPropertyFromDevice                                                                     */
/*                                                                                           */
/* This function can be used to request a value of any device property.                      */
/* There are many types of properties values. Refer to devpkey.h for more details.           */
/* Function SetupDiGetDeviceProperty() inside GetPropertyFromDevice() fills a property value */
/* in a correct format, but always returns the value as a pointer to a string of bytes.      */
/* The string of bytes must be casted outside of the function GetPropertyFromDevice()        */
/* to get a value in a format suitable for the given type of property.                       */
/*                                                                                           */
/*********************************************************************************************/
static bool GetPropertyFromDevice(
    DEVINST devInst,
    const DEVPROPKEY& PropertyKey,
    std::vector<BYTE>& PropertyData)
{
    unsigned long propertySize = 0;
    DEVPROPTYPE propertyType;

    // request a size, in bytes, required for a buffer in which property value will be stored.
    // CM_Get_DevNode_PropertyW() returns false and ERROR_INSUFFICIENT_BUFFER for the call
    CONFIGRET status = CM_Get_DevNode_PropertyW(devInst, &PropertyKey, &propertyType, NULL, &propertySize, 0);
    if (status != CR_BUFFER_SMALL)
    {
        IGC_ASSERT_MESSAGE(0, "CM_Get_DevNode_PropertyW() failed, status indicates error code");
        return false;
    }

    // allocate memory for the buffer
    PropertyData.clear();
    PropertyData.resize(propertySize, 0);

    // fill in the buffer with property value
    status = CM_Get_DevNode_PropertyW(devInst, &PropertyKey, &propertyType, &PropertyData[0], &propertySize, 0);
    if (status != CR_SUCCESS)
    {
        IGC_ASSERT_MESSAGE(0, "CM_Get_DevNode_PropertyW() failed, status indicates error code");
        PropertyData.clear();
        return false;
    }

    return true;
}

/********************************************************************************************/
/* ObtainDeviceInstance                                                                     */
/*                                                                                          */
/* Obtains a device instance for the Intel graphics adapter                                 */
/********************************************************************************************/
static HRESULT ObtainDeviceInstances(std::vector<DEVINST>* pDevInstances)
{
    CONFIGRET cr = CR_SUCCESS;

    ULONG DeviceIDListSize = 0;
    std::vector<wchar_t> DeviceIDList;
    std::vector<std::wstring> Devices;
    std::wstring IntelDeviceID;
    wchar_t DisplayDevClassGUID[40];

    do
    {
        if (pDevInstances == nullptr)
        {
            cr = CR_INVALID_POINTER;
            break;
        }

        StringFromGUID2(GUID_DEVCLASS_DISPLAY, DisplayDevClassGUID, sizeof(DisplayDevClassGUID));

        do
        {
            DeviceIDListSize = 0;
            cr = CM_Get_Device_ID_List_SizeW(
                &DeviceIDListSize,
                DisplayDevClassGUID,
                CM_GETIDLIST_FILTER_CLASS); // we don't want to use CM_GETIDLIST_FILTER_PRESENT in order to support safe mode.
            if (cr != CR_SUCCESS)
            {
                break;
            }

            if (DeviceIDList.max_size() >= DeviceIDListSize && DeviceIDListSize > 0)
                DeviceIDList.resize(DeviceIDListSize);
            else
            {
                return E_ABORT;
            }

            cr = CM_Get_Device_ID_ListW(
                DisplayDevClassGUID,
                DeviceIDList.data(),
                DeviceIDListSize,
                CM_GETIDLIST_FILTER_CLASS);

        } while (cr == CR_BUFFER_SMALL);

        if (cr != CR_SUCCESS)
        {
            break;
        }

        ConvertDoubleNullTermStringToVector(DeviceIDList, Devices);

        for (auto device : Devices)
        {
            if ((device.find(L"VEN_8086") != std::wstring::npos) ||
                (device.find(L"ven_8086") != std::wstring::npos))
            {
                // Found an intel device, add it to the list
                DEVINST devInst;
                CONFIGRET crLocateDevNode = CM_Locate_DevNodeW(
                    &devInst,
                    const_cast<wchar_t*>(device.c_str()),
                    CM_LOCATE_DEVNODE_NORMAL);
                if (crLocateDevNode == CR_NO_SUCH_DEVNODE)
                {
                    // There can be many drivers on the system from past installs.
                    // CR_NO_SUCH_DEVNODE is returned for drivers that don't match any devices.
                    // It's ok to skip these.
                    continue;
                }
                else
                {
                    if (crLocateDevNode == CR_SUCCESS)
                    {
                        pDevInstances->push_back(devInst);
                    }

                    // Update the overall status
                    cr |= crLocateDevNode;
                }
            }
        }

    } while (FALSE);

    return HRESULT_FROM_WIN32(CM_MapCrToWin32Err(cr, ERROR_FILE_NOT_FOUND));
}

/************************************************************************/
/* GetIntelDriverPaths                                                  */
/*                                                                      */
/* Build a list of Intel graphics driver installation paths.            */
/*                                                                      */
/************************************************************************/
static size_t GetIntelDriverPaths(std::vector<DEVINST>& drivers)
{
    drivers.clear();
    HRESULT hr = ObtainDeviceInstances(&drivers);
    if (FAILED(hr))
    {
        IGC_ASSERT_MESSAGE(0, "Failed to find any graphics device instances");
        return 0;
    }
    return drivers.size();
}


std::string getNewRegistryPath(DEVINST deviceInstance)
{
    DEVICE_INFO devInfo(deviceInstance);
    return devInfo.driverRegistryPath;
}
#endif

/*****************************************************************************\
ReadIGCEnv
\*****************************************************************************/
static bool ReadIGCEnv(
    const char*  pName,
    void*        pValue,
    unsigned int size )
{
    if( pName != NULL )
    {
        const char nameTag[] = "IGC_";
        std::string pKey = std::string(nameTag) + std::string(pName);

        const char* envVal = getenv(pKey.c_str());

        if( envVal != NULL )
        {
            if( size >= sizeof( unsigned int ) )
            {
                // Try integer conversion
                char* pStopped = nullptr;
                unsigned int *puVal = (unsigned int *)pValue;
                *puVal = strtoul(envVal, &pStopped, 0);
                if( pStopped == envVal + strlen(envVal) )
                {
                    return true;
                }
            }

            // Just return the string
            strncpy_s( (char*)pValue, size, envVal, size );

            return true;
        }
    }

    return false;
}

/*****************************************************************************\
ReadIGCRegistry
\*****************************************************************************/
static bool ReadIGCRegistry(
    const char*  pName,
    void*        pValue,
    unsigned int size ,
    std::string registrykeypath)
{
    // All platforms can retrieve settings from environment
    if( ReadIGCEnv( pName, pValue, size ) )
    {
        return true;
    }

#if defined _WIN32
    LONG success = ERROR_SUCCESS;
    HKEY uscKey;

    success = RegOpenKeyExA(
        HKEY_LOCAL_MACHINE,
        registrykeypath.c_str(),
        0,
        KEY_READ,
        &uscKey );

    if( ERROR_SUCCESS == success )
    {
        DWORD dwSize = size;
        success = RegQueryValueExA(
            uscKey,
            pName,
            NULL,
            NULL,
            (LPBYTE)pValue,
            &dwSize );

        RegCloseKey( uscKey );
    }
    return ( ERROR_SUCCESS == success );
#endif // defined _WIN32

    return false;
}

/*****************************************************************************\

Function:
    DumpIGCRegistryKeyDefinitions

Description:
    Dumps registry key definitions to an XML file.

Input:
    none

Output:
    none

\*****************************************************************************/
static const char* ConvertType(const char* flagType)
{
    if(strcmp(flagType, "bool") == 0)
    {
        return "bool";
    }
    if((strcmp(flagType, "int") == 0) || (strcmp(flagType, "DWORD") == 0))
    {
        return "DWORD";
    }
    if(strcmp(flagType, "debugString") == 0)
    {
        return "string";
    }
    return flagType;
}

#define DECLARE_IGC_REGKEY( dataType, regkeyName, defaultValue, descriptionText, releaseMode ) \
    fprintf(fp, "    <Key name=\"%s\" type=\"%s\" location=\"%s\" description=\"%s\" />\n",    \
        #regkeyName,                                                                           \
        ConvertType(#dataType),                                                                \
        "HKLM\\" IGC_REGISTRY_KEY,                                                             \
        descriptionText);
#define DECLARE_IGC_GROUP( groupName ) \
    if(!firstGroup)                    \
    {                                  \
        fprintf(fp, "  </Group>\n");   \
    }                                  \
    firstGroup = false;                \
    fprintf(fp, "  <Group name=\"%s\">\n", groupName);

void DumpIGCRegistryKeyDefinitions()
{
#ifdef _WIN32
    // Create the directory path

    iSTD::DirectoryCreate("C:\\Intel");
    iSTD::DirectoryCreate("C:\\Intel\\IGfx");
    iSTD::DirectoryCreate("C:\\Intel\\IGfx\\GfxRegistryManager");
    iSTD::DirectoryCreate("C:\\Intel\\IGfx\\GfxRegistryManager\\Keys");

    // Create the XML file to hold the debug variable definitions
    FILE* fp = fopen("C:\\Intel\\IGfx\\GfxRegistryManager\\Keys\\IGC.xml", "w");

    if (fp == NULL)
    {
        return;
    }
    bool firstGroup = true;
    // Generate the XML
    fprintf(fp, "<RegistryKeys>\n");
#include "igc_regkeys.def"
    fprintf(fp, "  </Group>\n");
    fprintf(fp, "</RegistryKeys>\n");

    fclose(fp);
    fp = NULL;
#endif
}
#undef DECLARE_IGC_REGKEY
#undef DECLARE_IGC_GROUP

void DumpIGCRegistryKeyDefinitions3(std::string driverRegistryPath, unsigned long pciBus, unsigned long pciDevice, unsigned long pciFunction)
{
#ifdef _WIN32
    // Create the directory path

    iSTD::DirectoryCreate("C:\\Intel");
    iSTD::DirectoryCreate("C:\\Intel\\IGfx");
    iSTD::DirectoryCreate("C:\\Intel\\IGfx\\GfxRegistryManager");
    iSTD::DirectoryCreate("C:\\Intel\\IGfx\\GfxRegistryManager\\Keys");

    if (driverRegistryPath.empty())
    {
        IGC_ASSERT_MESSAGE(0, "Failed to find the driver registry path, cannot create the debug variable XML file.");
        return;
    }

    std::string registryKeyPath = "HKLM\\SYSTEM\\ControlSet001\\Control\\Class\\" + driverRegistryPath + "\\IGC";

#define DECLARE_IGC_REGKEY( dataType, regkeyName, defaultValue, descriptionText, releaseMode ) \
    fprintf(fp, "    <Key name=\"%s\" type=\"%s\" location=\"%s\" description=\"%s\" />\n",    \
        #regkeyName,                                                                           \
        ConvertType(#dataType),                                                                \
        registryKeyPath.c_str(),                                                               \
        descriptionText);

#define DECLARE_IGC_GROUP( groupName ) \
    if(!firstGroup)                    \
    {                                  \
        fprintf(fp, "  </Group>\n");   \
    }                                  \
    firstGroup = false;                \
    fprintf(fp, "  <Group name=\"%s\">\n", groupName);

    std::string xmlPath = "C:\\Intel\\IGfx\\GfxRegistryManager\\Keys\\IGC." + std::to_string(pciBus) + "." + std::to_string(pciDevice) + "." + std::to_string(pciFunction) + ".xml";

    // Create the XML file to hold the debug variable definitions
    FILE* fp = fopen(xmlPath.c_str(), "w");

    if (fp == NULL)
    {
        return;
    }
    bool firstGroup = true;
    // Generate the XML
    fprintf(fp, "<RegistryKeys>\n");
#include "igc_regkeys.def"
    fprintf(fp, "  </Group>\n");
    fprintf(fp, "</RegistryKeys>\n");

    fclose(fp);
    fp = NULL;
#endif
}

#undef DECLARE_IGC_REGKEY
#undef DECLARE_IGC_GROUP


/// Function taken from LLVM CommandLine.cpp file
/// ParseCStringVector - Break INPUT up wherever one or more
/// whitespace characters are found, and store the resulting tokens in
/// OUTPUT. The tokens stored in OUTPUT are dynamically allocated
/// using strdup(), so it is the caller's responsibility to free()
/// them later.
///
static void ParseCStringVector(std::vector<char *> &OutputVector,
    const char *Input) {
    // Characters which will be treated as token separators:
    llvm::StringRef Delims = " \v\f\t\r\n";

    llvm::StringRef WorkStr(Input);
    while(!WorkStr.empty()) {
        // If the first character is a delimiter, strip them off.
        if(Delims.find(WorkStr[0]) != llvm::StringRef::npos) {
            size_t Pos = WorkStr.find_first_not_of(Delims);
            if(Pos == llvm::StringRef::npos) Pos = WorkStr.size();
            WorkStr = WorkStr.substr(Pos);
            continue;
        }

        // Find position of first delimiter.
        size_t Pos = WorkStr.find_first_of(Delims);
        if(Pos == llvm::StringRef::npos) Pos = WorkStr.size();

        // Everything from 0 to Pos is the next word to copy.
        char *NewStr = (char*)malloc(Pos + 1);
        memcpy_s(NewStr, Pos + 1, WorkStr.data(), Pos);
        NewStr[Pos] = 0;
        OutputVector.push_back(NewStr);

        WorkStr = WorkStr.substr(Pos);
    }
}

static void setRegkeyFromOption(
    std::string&     optionValue,
    const char* dataTypeName,
    const char* regkeyName,
    void*       pRegkeyVar,
    bool&       isKeySet)
{
    size_t sPos = optionValue.find(regkeyName);
    if(sPos == std::string::npos)
    {
        return;
    }
    size_t pos = sPos + strlen(regkeyName);
    pos = (pos < optionValue.size()) ? pos : std::string::npos;
    std::string vstring;
    if(pos != std::string::npos && optionValue.at(pos) == '=')
    {
        // Get value for this option, ',' is a value separator.
        sPos = pos + 1;
        pos = optionValue.find(',', sPos);
        size_t len = (pos == std::string::npos) ? pos : (pos - sPos);
        vstring = optionValue.substr(sPos, len);
    }

    bool isBool = (strcmp(dataTypeName, "bool") == 0);
    bool isInt = (strcmp(dataTypeName, "int") == 0) ||
        (strcmp(dataTypeName, "DWORD") == 0);
    bool isString = (strcmp(dataTypeName, "debugString") == 0);
    if(isBool)
    {
        bool bval = true;
        if(vstring.size() == 1)
        {
            bval = (vstring.at(0) == '0') ? false : true;
        }
        *((bool*)pRegkeyVar) = bval;
        isKeySet = true;
    }
    else if(isInt)
    {
        // ignore if there is no value
        if(!vstring.empty())
        {
            // assume vstring has the valid value!
            int intval = atoi(vstring.c_str());
            *((int*)pRegkeyVar) = intval;
            isKeySet = true;
        }
    }
    else if(isString)
    {
        // ignore if there is no value
        if(!vstring.empty())
        {
            char* s = (char*)pRegkeyVar;
            strcpy_s(s, vstring.size(), vstring.c_str());
            isKeySet = true;
        }
    }
}

static const char* GetOptionFile()
{
#if defined(_WIN64) || defined(_WIN32)
    return "c:\\Intel\\IGC\\debugFlags\\Options.txt";
#else
    return "/tmp/IntelIGC/debugFlags/Options.txt";
#endif
}

// parses this syntax:
// hash:abcdabcdabcdabcd-ffffffffffffffff,aaaaaaaaaaaaaaaa
static void ParseHashRange(std::string line, std::vector<HashRange>& ranges)
{
    size_t sPos = line.find("hash:");
    if(sPos == std::string::npos)
    {
        return;
    }
    // re-initialize ranges
    ranges.clear();
    std::string vString = line.substr(sPos + 5);
    std::string token;
    std::stringstream sline(vString);
    while(std::getline(sline, token, ','))
    {
        size_t dash = token.find('-');
        std::string start = token.substr(0, dash);
        std::string end;
        if(dash == std::string::npos)
        {
            end = start;
        }
        else
        {
            end = token.substr(dash + 1);
        }
        HashRange range;
        range.start = std::stoull(start, nullptr, 16);
        range.end = std::stoull(end, nullptr, 16);
        ranges.push_back(range);
    }

}

static void declareIGCKey(std::string& line, const char* dataType, const char* regkeyName, std::vector<HashRange>& hashes, SRegKeyVariableMetaData* regKey)
{
    bool isSet = false;
    debugString value = { 0 };
    setRegkeyFromOption(line, dataType, regkeyName, &value, isSet);
    if (isSet)
    {
        memcpy_s(regKey->m_string, sizeof(value), value, sizeof(value));
        regKey->hashes = hashes;
    }
}

static void LoadDebugFlagsFromFile()
{
    std::ifstream input(GetOptionFile());
    std::string line;
    std::vector<HashRange> hashes;

    while (std::getline(input, line)) {
        ParseHashRange(line, hashes);
#define DECLARE_IGC_REGKEY(dataType, regkeyName, defaultValue, description, releaseMode)         \
{                                                                                   \
    declareIGCKey(line, #dataType, #regkeyName, hashes, &(g_RegKeyList.regkeyName));\
}
#include "igc_regkeys.def"
#undef DECLARE_IGC_REGKEY

    }
}

thread_local unsigned long long g_CurrentShaderHash = 0;
bool CheckHashRange(const std::vector<HashRange>& hashes)
{
    if(hashes.empty())
    {
        return true;
    }
    for(auto it : hashes)
    {
        if(g_CurrentShaderHash >= it.start && g_CurrentShaderHash <= it.end)
        {
            return true;
        }
    }
    return false;
}

static void LoadFromRegKeyOrEnvVarOrOptions(const std::string& options = "", bool* RegFlagNameError = nullptr, std::string registrykeypath = IGC_REGISTRY_KEY)
{
    SRegKeyVariableMetaData* pRegKeyVariable = (SRegKeyVariableMetaData*)&g_RegKeyList;
    unsigned NUM_REGKEY_ENTRIES = sizeof(SRegKeysList) / sizeof(SRegKeyVariableMetaData);
    for (DWORD i = 0; i < NUM_REGKEY_ENTRIES; i++)
    {
        debugString value = { 0 };
        const char* name = pRegKeyVariable[i].GetName();
        std::string nameWithEqual = name;
        nameWithEqual = nameWithEqual + "=";

        bool isSet = ReadIGCRegistry(
            name,
            &value,
            sizeof(value), registrykeypath);

        if (isSet)
        {
            memcpy_s(pRegKeyVariable[i].m_string, sizeof(value), value, sizeof(value));
        }

        debugString valueFromOptions = { 0 };
        std::size_t found = options.find(nameWithEqual);

        if (found != std::string::npos)
        {
            std::size_t foundComma = options.find(',', found);
            if (foundComma != std::string::npos)
            {
                if (found == 0 || options[found - 1] == ' ' || options[found - 1] == ',')
                {
                    std::string token = options.substr(found + nameWithEqual.size(), foundComma - (found + nameWithEqual.size()));
                    unsigned int size = sizeof(value);
                    void* pValueFromOptions = &valueFromOptions;

                    const char* envValFromOptions = token.c_str();
                    bool valueIsInt = false;
                    if (envValFromOptions != NULL)
                    {
                        if (size >= sizeof(unsigned int))
                        {
                            // Try integer conversion
                            char* pStopped = nullptr;
                            unsigned int* puValFromOptions = (unsigned int*)pValueFromOptions;
                            *puValFromOptions = strtoul(envValFromOptions, &pStopped, 0);
                            if (pStopped == envValFromOptions + strlen(envValFromOptions))
                            {
                                valueIsInt = true;
                            }
                        }
                        if (!valueIsInt)
                        {
                            // Just return the string
                            strncpy_s((char*)pValueFromOptions, size, envValFromOptions, size);
                        }
                    }
                    memcpy_s(pRegKeyVariable[i].m_string, sizeof(valueFromOptions), valueFromOptions, sizeof(valueFromOptions));
                }
                else if(RegFlagNameError != nullptr)
                {
                    *RegFlagNameError = true;
                }
            }
        }
    }
}

void SetCurrentDebugHash(unsigned long long hash)
{
    g_CurrentShaderHash = hash;
}

/*****************************************************************************\

Function:
    LoadRegistryKeys

Description:
    Loads registry variables from the registry

Input:
    None

Output:
    None

\*****************************************************************************/
void LoadRegistryKeys(const std::string& options, bool *RegFlagNameError)
{
    // only load the debug flags once before compiling to avoid any multi-threading issue
    static std::mutex loadFlags;
    static volatile bool flagsSet = false;
    loadFlags.lock();
    if(!flagsSet)
    {
        flagsSet = true;
        // dump out IGC.xml for the registry manager
#if defined(_WIN64) || defined(_WIN32)
        std::vector<DEVINST> drivers;
        std::list<std::string> registrypaths;
        GetIntelDriverPaths(drivers);
        for (auto driverInfo : drivers)
        {
            std::string driverStoreRegKeyPath = getNewRegistryPath(driverInfo);
            std::string registryKeyPath = "SYSTEM\\ControlSet001\\Control\\Class\\" + driverStoreRegKeyPath + "\\IGC";
            LoadFromRegKeyOrEnvVarOrOptions(options, RegFlagNameError, registryKeyPath);
        }
#endif
        //DumpIGCRegistryKeyDefinitions();
        LoadDebugFlagsFromFile();
        LoadFromRegKeyOrEnvVarOrOptions(options, RegFlagNameError);

        if(IGC_IS_FLAG_ENABLED(LLVMCommandLine))
        {
            std::vector<char*> args;
            args.push_back((char *)("IGC"));
            ParseCStringVector(args, IGC_GET_REGKEYSTRING(LLVMCommandLine));
            llvm::cl::ParseCommandLineOptions(args.size(), &args[0]);
        }

        if(IGC_IS_FLAG_ENABLED(DisableIGCOptimizations))
        {
            IGC_SET_FLAG_VALUE(DisableLLVMGenericOptimizations, true);
            IGC_SET_FLAG_VALUE(DisableCodeSinking, true);
            IGC_SET_FLAG_VALUE(DisableDeSSA, true);
            //disable now until we figure out the issue
            //IGC_SET_FLAG_VALUE(DisablePayloadCoalescing, true);
            IGC_SET_FLAG_VALUE(DisableSendS, true);
            IGC_SET_FLAG_VALUE(EnableVISANoSchedule, true);
            IGC_SET_FLAG_VALUE(DisableUniformAnalysis, true);
            IGC_SET_FLAG_VALUE(DisablePushConstant, true);
            IGC_SET_FLAG_VALUE(DisableConstantCoalescing, true);
            IGC_SET_FLAG_VALUE(DisableURBWriteMerge, true);
            IGC_SET_FLAG_VALUE(DisableCodeHoisting, true);
            IGC_SET_FLAG_VALUE(DisableEmptyBlockRemoval, true);
            IGC_SET_FLAG_VALUE(DisableSIMD32Slicing, true);
            IGC_SET_FLAG_VALUE(DisableCSEL, true);
            IGC_SET_FLAG_VALUE(DisableFlagOpt, true);
            IGC_SET_FLAG_VALUE(DisableScalarAtomics, true);
        }


        if(IGC_IS_FLAG_ENABLED(ShaderDumpEnableAll))
        {
            IGC_SET_FLAG_VALUE(ShaderDumpEnable, true);
            IGC_SET_FLAG_VALUE(EnableVISASlowpath, true);
            IGC_SET_FLAG_VALUE(EnableVISADumpCommonISA, true);
        }

        if(IGC_IS_FLAG_ENABLED(ShaderDumpEnable))
        {
            IGC_SET_FLAG_VALUE(DumpLLVMIR, true);
            IGC_SET_FLAG_VALUE(EnableCosDump, true);
            IGC_SET_FLAG_VALUE(DumpOCLProgramInfo, true);
            IGC_SET_FLAG_VALUE(EnableVISAOutput, true);
            IGC_SET_FLAG_VALUE(EnableVISABinary, true);
            IGC_SET_FLAG_VALUE(EnableVISADumpCommonISA, true);
            IGC_SET_FLAG_VALUE(EnableCapsDump, true);
            IGC_SET_FLAG_VALUE(DumpPatchTokens, true);
        }

        if (IGC_IS_FLAG_ENABLED(DumpTimeStatsPerPass) ||
            IGC_IS_FLAG_ENABLED(DumpTimeStatsCoarse))
        {
            IGC_SET_FLAG_VALUE(DumpTimeStats, true);
            IGC::Debug::SetDebugFlag(IGC::Debug::DebugFlag::TIME_STATS_PER_SHADER, true);
        }

        if (IGC_IS_FLAG_ENABLED(DumpTimeStats))
        {
            // Need to turn on this setting so per-shader .csv is generated
            IGC::Debug::SetDebugFlag(IGC::Debug::DebugFlag::TIME_STATS_PER_SHADER, true);
        }

        switch(IGC_GET_FLAG_VALUE(ForceOCLSIMDWidth))
        {
        case 32:
            IGC_SET_FLAG_VALUE(EnableOCLSIMD32, true);
            IGC_SET_FLAG_VALUE(EnableOCLSIMD16, false);
            break;
        case 16:
            IGC_SET_FLAG_VALUE(EnableOCLSIMD32, false);
            IGC_SET_FLAG_VALUE(EnableOCLSIMD16, true);
            break;
        case 8:
            IGC_SET_FLAG_VALUE(EnableOCLSIMD32, false);
            IGC_SET_FLAG_VALUE(EnableOCLSIMD16, false);
            break;
        default:
            // Non-valid value is ignored (using default).
            IGC_SET_FLAG_VALUE(ForceOCLSIMDWidth, 0);
        }
    }
    loadFlags.unlock();
}
#endif
