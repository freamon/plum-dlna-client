/**
 *  @section LICENSE
 *
 *  PickleLauncher
 *  Copyright (C) 2010-2011 Scott Smith
 * 
 *  PLUM
 *  Copyright (C) 2013 Andy Slater (freamon)
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  @section LOCATION
 */
 
#ifndef CPLUM_H
#define CPLUM_H

#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <curl/curl.h>

#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <pthread.h>

#include "cbase.h"

#define NO_MS_AVAILABLE_LABEL	"<no media servers available>"		/** Ascii text for display if no media servers are available. */
#define NO_MS_ENTRIES_LABEL		"<media server has no files>"		/** Ascii text for display if active media server is empty */

// how many UDP packets to send every 10 seconds, searching for media servers
#define PACKET_COUNT			3

#define SSDP_MULTICAST      	"239.255.255.250"
#define SSDP_PORT           	1900
#define RESPONSE_BUFFER_LEN 	1024


// used when parsing xml
#define GET_NOTHING				0
#define GET_UDN					1
#define GET_FRIENDLY_NAME		2
#define GET_SERVICE_TYPE		3
#define GET_CONTENT_DIRECTORY	4
#define GET_CONTROL_URL			5

#define GET_BROWSE_RESULT		6

#define GET_CONTAINER			7
#define GET_CONTAINER_TITLE		8

#define GET_ITEM				9
#define GET_ITEM_TITLE		   10
#define GET_RES				   11
#define GET_CLASS			   12
#define GET_ARTWORK			   13


struct media_server_info_t {
    media_server_info_t() : location(""), baseURL(""), UDN(""), friendlyName(""), controlURL(""), isActive(false) {};
    string location;
    string baseURL;
    string UDN;
    string friendlyName;
    string controlURL;
    bool isActive;
};



struct browse_response_t {
    browse_response_t() : id(""), childCount(""), title(""), url(""), upnpclass(""), albumArtURI(""), albumArtFilename(""), albumArtDownload(false), isDirectory(true), isSelected(false) {};
    string id;
    string childCount;
    string title;
    string url;
    string upnpclass;
    string albumArtURI;
    string albumArtFilename;
    bool albumArtDownload;
    bool isDirectory;
    bool isSelected;
};

struct directory_stack_t {
	directory_stack_t() : filePath(""), objectID(""), requestedCount("") {};
	string filePath;
	string objectID;
	string requestedCount;
};

struct readarg_t {
	readarg_t(): buf(NULL), len(0), pos(0) {}; 
	char *buf;
	int len;
	int pos;
};

struct artwork_download_t {
		artwork_download_t(): URI(""), filename("") {}; 
		string URI;
		string filename;
};

class CPlum : public CBase
{
    public:
        /** Constructor. */
        CPlum();
        /** Destructor. */
        virtual ~CPlum();
        
        int8_t StartUp();
        void Reset();
        void Shutdown();
        
        bool CheckResponses(void);
        void SetActiveServer(string UDN);
        bool GetActiveServer(void);
        
        void BrowseActiveMediaServer();
        bool DirectoryUp();
        bool DirectoryDown(string id);
        
        string GetFilePath();
        bool IsPlaylist();
        bool CreatePlaylist();
        
		vector<browse_response_t> browseResponses;
        vector<media_server_info_t> mediaServers;
        vector<string> helpModeEntries;

	private:
	
		
		void parseXML(string content);
        void print_element_names(xmlNode * a_node);
		
		CURL *curlSearchHandle;
		CURL *curlBrowseHandle;
		CURLcode res;
		
		struct timeval timeout1;
		char buffer1[RESPONSE_BUFFER_LEN];
		unsigned int len1;
		struct sockaddr clientsock1;
		unsigned int socklen1;
		media_server_info_t currentMediaServer;
		
		bool newLocation1;
		string location1;
		string baseURL1;
		
		readarg_t rarg;
		struct curl_slist *httpHeader;
		
		string postDataHeader;
		string postDataMiddle;
		string postDataFooter;
		
		
		browse_response_t currentBrowseResponse;
		
		int getNode;
		
		string content1;
		
		vector<directory_stack_t> directoryStack;
		directory_stack_t currentDirectory;
		
		bool isPlaylist;
		
		string Result;
		
		bool PMS_hack;
};



#endif // CPLUM_H
