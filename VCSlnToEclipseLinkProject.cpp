// VCSlnToEclipseLinkProject.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "tinyxml2.h"

#include <string>
#include <vector>
#include <iostream>

#include <filesystem>
#include <regex>

#include "boost/assert.hpp"
#include "boost/tokenizer.hpp"

namespace fs = std::tr2::sys;

#define ECLIPSE_PROJ_linkedResources_NAME "linkedResources"
#define ECLIPSE_PROJ_LINK_NAME "link"
#define ECLIPSE_PROJ_LINK_name_NAME "name"
#define ECLIPSE_PROJ_LINK_type_NAME	"type"
#define ECLIPSE_PROJ_LINK_locationURI_NAME "locationURI"

#define VC_PROJ_Root_NAME "VisualStudioProject"
#define VC_PROJ_Files_NAME	"Files"
#define VC_PROJ_Filter_NAME "Filter"
#define VC_PROJ_File_NAME	"File"
struct VC9ProjFileItem
{
	std::string filter;
	fs::path filename;
	bool bRelativePath;
};

typedef std::vector<VC9ProjFileItem>	VC9ProjFileVector;

#include <boost/filesystem.hpp>
fs::path canonical(const fs::path &p, const fs::path &basePath)
{
	return fs::path(boost::filesystem::canonical(boost::filesystem::path(p.string().c_str()), basePath.string()).string());
}

static void load_vcproj_files(tinyxml2::XMLElement *pFiles, const std::string& filterName, VC9ProjFileVector &files)
{
	for (auto pElem = pFiles->FirstChildElement();
		nullptr != pElem; pElem = pElem->NextSiblingElement())
	{
		if (pElem->Name() == std::string(VC_PROJ_Filter_NAME))
		{
#define VC_PROJ_FILTER_NAME "Name"
			load_vcproj_files(pElem, pElem->Attribute(VC_PROJ_FILTER_NAME), files);
		}

		VC9ProjFileItem item;
		item.filter = filterName;

		const char* pPath = pElem->Attribute("RelativePath");

		if (NULL != pPath && *pPath != '\0')
		{
			item.bRelativePath = true;
			item.filename = pPath;

			files.push_back(item);
		}

		BOOST_ASSERT(false);

	}
}

static bool load_vcproj_file_to_eclipse(const fs::path &vcFileName, tinyxml2::XMLDocument *pEclipse)
{
	if (!is_regular_file(vcFileName))
	{
		return false;
	}

	tinyxml2::XMLDocument vcElem;
	if (tinyxml2::XML_SUCCESS != vcElem.LoadFile(vcFileName.string().c_str()))
	{
		return false;
	}

	tinyxml2::XMLElement *pFiles = vcElem.FirstChildElement(VC_PROJ_Root_NAME)->FirstChildElement(VC_PROJ_Files_NAME);

	VC9ProjFileVector vc9projFiles;
	load_vcproj_files(pFiles, "", vc9projFiles);

	std::for_each(vc9projFiles.begin(), vc9projFiles.end(), [&vcFileName](VC9ProjFileItem &vc9File){ vc9File.filename = ::canonical(vc9File.filename, vcFileName);  });

	tinyxml2::XMLElement *pLinkedResourcesElem = pEclipse->FirstChildElement(ECLIPSE_PROJ_linkedResources_NAME);

	if (NULL == pLinkedResourcesElem)
		return false;


	for (auto itFile = vc9projFiles.begin();
		itFile != vc9projFiles.end(); ++itFile)
	{
		tinyxml2::XMLElement *pLinkElem = pEclipse->NewElement(ECLIPSE_PROJ_LINK_NAME);

		pLinkedResourcesElem->InsertEndChild(pLinkElem);

		tinyxml2::XMLElement *pLinkNameElem = pEclipse->NewElement(ECLIPSE_PROJ_LINK_name_NAME);
		pLinkElem->InsertEndChild(pLinkNameElem);

		tinyxml2::XMLElement *pLinkTypeElem = pEclipse->NewElement(ECLIPSE_PROJ_LINK_type_NAME);
		pLinkElem->InsertEndChild(pLinkTypeElem);

		tinyxml2::XMLElement *pLinkURIElem = pEclipse->NewElement(ECLIPSE_PROJ_LINK_locationURI_NAME);
		pLinkElem->InsertEndChild(pLinkURIElem);
	}
	//elem

	return true;
}


typedef std::vector<fs::path>	VC9ProjPathVector;



static bool find_vc9proj_files(const fs::path &vsSlnName, VC9ProjPathVector &projFiles)
{
	//Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "ComGacGas", "..\..\game\gac_gas\ComGacGas\ComGacGas.vcproj", "{67A10648-05F2-434B-943F-AA2B2879F317}"
	std::regex re("Project[^,]+,\\s*\"([^,]+\\.vcproj)\",.+$");

	std::ifstream iftrm(vsSlnName.string().c_str());

	if (!iftrm)
		return false;

	std::string line;
	while (std::getline(iftrm, line))
	{
		const std::string ;

		std::smatch result;
		if (std::regex_match(line, result, re))
		{
			const fs::path pp(*(result.begin() + 1));
			const fs::path finalPath = ::canonical(pp, vsSlnName.parent_path());
			if (fs::is_regular_file(finalPath))
			{
				projFiles.push_back(finalPath);
			}
			
		}
	}


	return true;
}

int main(int argc, char** argv)
{
	if (argc < 1)
	{
		std::cout << "need more argc" << std::endl;
		return -1;
	}

	fs::path vsSlnFileName(argv[1]);
	fs::path eclipseFileName(argv[2]);

	if (!is_regular_file(vsSlnFileName))
	{
		std::cout << "not valid sln file" << std::endl;
		return -2;
	}

	if (!is_regular_file(eclipseFileName))
	{
		std::cout << "not valid eclipse file" << std::endl;
		return -3;
	}

	tinyxml2::XMLDocument doc;
	if (tinyxml2::XML_SUCCESS != doc.LoadFile(eclipseFileName.string().c_str()))
	{
		std::cout << "load eclpise file to xml failed" << std::endl;
		return -4;
	}

	VC9ProjPathVector vc9ProjFiles;
	find_vc9proj_files(vsSlnFileName, vc9ProjFiles);

	std::cout << "find vcproj : " << std::endl;

	std::for_each(vc9ProjFiles.begin(), vc9ProjFiles.end(), [](const fs::path &vcFile){std::cout << vcFile << std::endl; });
	std::for_each(std::begin(vc9ProjFiles), std::end(vc9ProjFiles), [&doc](const fs::path &vcFile){load_vcproj_file_to_eclipse(vcFile, &doc); });
	


	return 0;
}

