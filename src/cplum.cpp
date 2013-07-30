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
 
#include "cplum.h"

/* BIG FAT TODO: c code -> c++ code
 * Lots of this uses the C versions rather than the C++ versions
 * Stuff like threads, mutexes, xml, and curl
 * I'm a newb. */
 
/* see what I mean, there's even global variables in here. */ 

/* mutexes and variables changed within them */
/* mutex1 is related to the thread that searches for media servers */
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
bool runThread = true;
int sock1;
struct sockaddr_in sockname;
int socknameSize = sizeof(struct sockaddr_in);
fd_set fds1;

artwork_download_t artworkDownload;
vector<artwork_download_t> artworkDownloadStack;

/* mutex2 is related to the thread that download album art */
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
bool downloadThreadActive = false;


/* temp fix: I should be getting this from Config class */
string previewsPath = "previews";  


/* Curl callback function that writes out album art image data */
size_t write_preview(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written;
    written = fwrite(ptr, size, nmemb, stream);
    return written;
}

/* Curl callback function that writes out responses to media server requests */
size_t write_data(char* data, size_t size, size_t nmemb, string* buffer) {
  if (buffer != NULL) {
    buffer->append(data, size*nmemb);
    return size*nmemb;
  }
  return 0;
}

/* Curl callback function that writes out responses to media server requests */
size_t readcb(void *ptr, size_t size, size_t nitems, void *stream)
{
	readarg_t *rarg = (readarg_t *)stream;
	unsigned int len = rarg->len - rarg->pos;
	if (len > size * nitems)
		len = size * nitems;
	memcpy(ptr, rarg->buf + rarg->pos, len);
	rarg->pos += len;
	//printf("readcb: %d bytes\n", len);
	return len;
}

void* DownloadArtwork(void*)
{
	CURL *curl;
    FILE *fp;
    CURLcode res;
    
    curl = curl_easy_init();
    if (curl) {
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_preview);
		for (unsigned int i = 0; i < artworkDownloadStack.size(); i++)  {
			
			fp = fopen(artworkDownloadStack.at(i).filename.c_str(),"wb");
			if (fp)  {
				curl_easy_setopt(curl, CURLOPT_URL, artworkDownloadStack.at(i).URI.c_str());
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
				res = curl_easy_perform(curl);
				fclose(fp);
				if(res != CURLE_OK)  {
					printf("curl error\n");
					remove(artworkDownloadStack.at(i).filename.c_str());
				}
				
			}
		}
		
		curl_easy_cleanup(curl);
	}
	
	pthread_mutex_lock( &mutex2 );
	downloadThreadActive = false;
	pthread_mutex_unlock( &mutex2 );
	
	pthread_exit(NULL);
	
}

void* SearchForMediaServers(void*)
{
	/* this should open targets.data for ST */
	/* if (useFavouriteServer == true)  {
		fileIn.open("targets.data", ios_base::in);
		if (fileIn)  {
			while(true)  {
				string line;
				getline(fileIn, line);
				if(fileIn.fail())
					break;
				line += "\n";
				mSearchTarget.push_back(line.c_str());
			}
		}  else
			useFavouriteServer = false;
	}
			
	if (useFavouriteServer == false)  {
		mSearchTarget.push_back("ST:upnp:rootdevice\r\n");
		mSearchTarget.push_back("ST:urn:schemas-upnp-org:device:MediaServer:1\r\n");
	}*/
	
	
	char search[] = "M-SEARCH * HTTP/1.1\r\n"
					"Host: 239.255.255.250:1900\r\n"
					"Man: \"ssdp:discover\"\r\n"
					"ST:urn:schemas-upnp-org:device:MediaServer:1\r\n"
					"MX:1\r\n"
					"\r\n";
					
	int searchLength = sizeof(search);				
					
	
				
	
	sockname.sin_family=AF_INET;
	sockname.sin_port=htons(SSDP_PORT);
	sockname.sin_addr.s_addr=inet_addr(SSDP_MULTICAST);
	
	
	

	
	while (true)  {
		for (int i = 0; i < PACKET_COUNT; i++)  {
			pthread_mutex_lock( &mutex1 );
			//printf("sending search request\n");
			int ret = sendto(sock1, search, searchLength, 0, (struct sockaddr*) &sockname, 
							 socknameSize);
			pthread_mutex_unlock( &mutex1 );
			if(ret != searchLength)
					printf("sendto failed\n");
					// msg: are you online?
		}	
		
		
			//printf("sleeping for 10 secs\n");
			sleep(10);
		
		/* this does exit - it does stop sending search packetes *
		 * but it doesn't show printf output until after program closed.
		 * Maybe no pthread_join() */
		if (runThread == false)  {
			//printf("exiting thread");
			//
			pthread_exit(NULL);
		}
		
	}
}

CPlum::CPlum() : CBase()
{
}
 
CPlum::~CPlum()
{
}

int8_t CPlum::StartUp()
{
	ifstream fileIn;
	
	sock1 = socket(PF_INET, SOCK_DGRAM, 0);
	if ( sock1 == -1)  { 
		printf("socket() failed\n");
		exit(1);
	}
	
	Reset();
	
	
		
	timeout1.tv_sec=0;
	timeout1.tv_usec=0;
	
	len1 = RESPONSE_BUFFER_LEN;
	socklen1=sizeof(clientsock1);
	
	httpHeader = NULL;
	
	if ( curl_global_init(CURL_GLOBAL_NOTHING) != 0)  {
		printf("global curl problem\n");
		exit(-1);
	}
	curlSearchHandle = curl_easy_init();
	curlBrowseHandle = curl_easy_init();
	
	if (curlSearchHandle)  {
		// Define callback function that will be called...
		curl_easy_setopt(curlSearchHandle, CURLOPT_WRITEFUNCTION, &write_data);
		// and pass the object this function will stuff the content in.
		curl_easy_setopt(curlSearchHandle, CURLOPT_WRITEDATA, &content1);
	}
	
	
	if (curlBrowseHandle)  {
		httpHeader = curl_slist_append(httpHeader, "SOAPAction: \"urn:schemas-upnp-org:service:ContentDirectory:1#Browse\"");
		                                            //SOAPAction:  "urn:schemas-upnp-org:service:ContentDirectory:1#Browse"
		httpHeader = curl_slist_append(httpHeader, "Content-Type: text/xml; charset=\"utf-8\"");
		
		
		
		// Define callback function that will be called...
		curl_easy_setopt(curlBrowseHandle, CURLOPT_WRITEFUNCTION, &write_data);
		// and pass the object this function will stuff the content in.
		curl_easy_setopt(curlBrowseHandle, CURLOPT_WRITEDATA, &content1);
		
		curl_easy_setopt(curlBrowseHandle, CURLOPT_READFUNCTION, readcb);
		//curl_easy_setopt(curlBrowseHandle, CURLOPT_READDATA, &rarg);
		
		curl_easy_setopt(curlBrowseHandle, CURLOPT_POST, 1);
		curl_easy_setopt(curlBrowseHandle, CURLOPT_HTTPHEADER, httpHeader);
		//curl_easy_setopt(curlBrowseHandle, CURLOPT_POSTFIELDSIZE, rarg.len);
		
	}
	
	postDataHeader = 		 "<?xml version='1.0' encoding='utf-8'?>"
							 "<s:Envelope s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\" xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\">"
							 "<s:Body>"
							 "<u:Browse xmlns:u=\"urn:schemas-upnp-org:service:ContentDirectory:1\">"
							 "<ObjectID>";
	
	postDataMiddle =		 "</ObjectID>"
							 "<BrowseFlag>BrowseDirectChildren</BrowseFlag>"
							 //"<Filter>dc:date,upnp:genre,res,res@duration,res@size,upnp:albumArtURI,upnp:album,upnp:artist,upnp:author,searchable,childCount</Filter>"
							 "<Filter>*</Filter>"
							 "<StartingIndex>0</StartingIndex>"
							 "<RequestedCount>";
	
	postDataFooter = 		 "</RequestedCount>"
							 "<SortCriteria></SortCriteria>"
							 "</u:Browse>"
							 "</s:Body>"
							 "</s:Envelope>";
							 
	
	helpModeEntries.push_back("SCROLL DOWN");
	helpModeEntries.push_back("SCROLL UP");
	helpModeEntries.push_back("PAGE DOWN");
	helpModeEntries.push_back("PAGE UP");
	helpModeEntries.push_back("PARENT DIRECTORY");
	helpModeEntries.push_back("SWITCH BETWEEN LOCAL AND NETWORK FILESYSTEM");
	helpModeEntries.push_back("PERFORM NEW SEARCH FOR MEDIA SERVERS");
	helpModeEntries.push_back("TOGGLE HELP MODE ON AND OFF");
	helpModeEntries.push_back("SELECT");
	helpModeEntries.push_back("SELECT ALL");
	helpModeEntries.push_back("QUIT");
	helpModeEntries.push_back("(INFO)");
	
	isPlaylist = false;
	
	
	 						 
	
	return 0;
}

void CPlum::Reset()
{
	int rc1;
	pthread_t thread1;
	
	if( (rc1=pthread_create( &thread1, NULL, SearchForMediaServers, NULL)) )  {
		printf("Thread creation failed: %d\n", rc1);
	}
	
	directoryStack.clear();
	currentDirectory.filePath = "Searching For Media Servers ...";
	currentDirectory.requestedCount = "-1";
	currentDirectory.objectID = "-1";
	directoryStack.push_back(currentDirectory);
	
	getNode = GET_NOTHING;
	content1 = "";		
	newLocation1 = true;
	
	currentMediaServer.isActive = false;
	
	Result = "";
	PMS_hack = false;
}

void CPlum::Shutdown()
{
	// write out search target vector
	//ofstream fileOut("targets.data", ios_base::trunc);
	
	//for (unsigned int i = 0; i < mSearchTarget.size(); i++)  {
	//	fileOut << mSearchTarget.at(i).c_str();
	//}
	
	close(sock1);
	
	curl_easy_cleanup(curlBrowseHandle);
	curl_easy_cleanup(curlSearchHandle);
	curl_global_cleanup();
	
}



bool CPlum::CheckResponses(void)
{
	if (currentMediaServer.isActive == true) 
		return false;
	
	
	pthread_mutex_lock( &mutex1 );
	FD_ZERO(&fds1);
	FD_SET(sock1, &fds1);
	
	bool gotResponse = false;
	
	if(select(sock1+1, &fds1, NULL, NULL, &timeout1) < 0)
				Log( "Error: select() failed\n" );
	
	if(FD_ISSET(sock1, &fds1))  {
		unsigned int len = recvfrom(sock1, buffer1, len1, 0, 
					   &clientsock1, &socklen1);
		if( len == (unsigned int)-1 )  {
			Log( "Error: recvfrom() failed\n" );
			return false;
		}
		buffer1[len]='\0';
		// Check the HTTP response code 
		if(strncmp(buffer1, "HTTP/1.1 200 OK", 12) != 0)  {
				Log( "Error: parsing ssdp failed\n" );
				return false;
		}
		
		gotResponse = true;
	}
	pthread_mutex_unlock( &mutex1 );
			
	if (gotResponse == true)  {	
		//printf("buffer is %s\n", buffer1);
			
		char* p = strstr(buffer1, "LOCATION: ");
		if (p == NULL)
			p = strstr(buffer1, "Location: ");
			
		if (p == NULL)  {
			//printf("p is still null\n");
			return false;
		}
		p+=10;
		char* q = p;
		while (*q != '\r')
			q++;
		*q = '\0';
		location1 = p;
		
		newLocation1 = true;
		
		//printf("location is %s, baseURL is %s\n", location.c_str(), baseURL.c_str());
		
		for (unsigned int i = 0; i < mediaServers.size(); i++)  {
			if (location1 == mediaServers.at(i).location)
				newLocation1 = false;
		}
		
		if (newLocation1 == true)  {
			
			q = p;
			for (int i=0; i<3; i++)
				while (*++q != '/');
			*q = '\0';
			baseURL1 = p;
			
			currentMediaServer.isActive = false;
			currentMediaServer.location = location1;
			currentMediaServer.baseURL = baseURL1;
			
			
			content1 = "";
		
		
			if(curlSearchHandle) {
				curl_easy_setopt(curlSearchHandle, CURLOPT_URL, location1.c_str());
				
				// Perform the request, res will get the return code 
				res = curl_easy_perform(curlSearchHandle);
				// Check for errors 
				if(res != CURLE_OK)  {
					fprintf(stderr, "curl_easy_perform() failed: %s\n",
							curl_easy_strerror(res));
					return false;
				}
 
				// always cleanup 
				
			}
		
			if (content1.length() > 0)  {
				// change parseXML to return true / false
				parseXML(content1);
				return true;
			}
			
		}
	}
	
	return false;
	
}

void CPlum::SetActiveServer(string UDN)
{
	for (unsigned int i = 0; i < mediaServers.size(); i++)  {
		if (mediaServers.at(i).UDN == UDN)  {
			//printf("setting active server to %s\n", currentMediaServer.friendlyName.c_str());
			mediaServers.at(i).isActive = true;
			currentMediaServer = mediaServers.at(i);
			
			if (! strncmp(currentMediaServer.friendlyName.c_str(), "PS3", 3) )  {
				//printf("PS3 hack\n");
				PMS_hack = true;
			}
			
			currentDirectory.filePath = currentMediaServer.friendlyName;
			currentDirectory.objectID = "0";
			currentDirectory.requestedCount = "30";
			directoryStack.push_back(currentDirectory);
						
			pthread_mutex_lock( &mutex1 );
			//printf("putting runThread to false\n");
			runThread = false;
			pthread_mutex_unlock( &mutex1 );
		}
		else  
			mediaServers.at(i).isActive = false;
	}
}

bool CPlum::GetActiveServer()
{
	if (currentMediaServer.isActive == true)
		return true;
	else
		return false;
}

void CPlum::BrowseActiveMediaServer()
{
	int rc1;
	pthread_t thread2;
	
	
	/* this should be configurable */
	bool getAlbumArt = true;
	
	artworkDownloadStack.clear();
	
	browseResponses.clear();
	
	string postData = postDataHeader + directoryStack.back().objectID + postDataMiddle + directoryStack.back().requestedCount + postDataFooter;
	string fullControlURL = currentMediaServer.baseURL + currentMediaServer.controlURL;
	
	//printf("full control url is %s\n", fullControlURL.c_str());
	
	rarg.buf = (char*)postData.c_str();
	rarg.len = postData.size();
	rarg.pos = 0;
	
	content1 = "";
	
	if (curlBrowseHandle)  {
		curl_easy_setopt(curlBrowseHandle, CURLOPT_URL, fullControlURL.c_str());
		curl_easy_setopt(curlBrowseHandle, CURLOPT_POSTFIELDSIZE, rarg.len);
		curl_easy_setopt(curlBrowseHandle, CURLOPT_READDATA, &rarg);
		curl_easy_setopt(curlBrowseHandle, CURLOPT_USERAGENT, "PLUM");
		
		curl_easy_perform(curlBrowseHandle);
	}
	
	
	if (content1.length() > 0)  {
		
		/*for (unsigned int i = 0; i < content1.length(); i++)  {
			if (content1[i] == '&' && content1[i+1] == 'l' && content1[i+2] == 't' && content1[i+3] == ';') 
				content1.replace(i, 4, "<");
			if (content1[i] == '&' && content1[i+1] == 'g' && content1[i+2] == 't' && content1[i+3] == ';') 
				content1.replace(i, 4, ">");
		}*/
		
		parseXML(content1);
		//printf("content is %s\n", content1.c_str());
	}
	
	if (Result.length() > 0)  {
		
		for (unsigned int i = 0; i < content1.length(); i++)  {
			if (content1[i] == '&' && content1[i+1] == 'l' && content1[i+2] == 't' && content1[i+3] == ';') 
				content1.replace(i, 4, "<");
			if (content1[i] == '&' && content1[i+1] == 'g' && content1[i+2] == 't' && content1[i+3] == ';') 
				content1.replace(i, 4, ">");
			if (content1[i] == '&' && content1[i+1] == 'q' && content1[i+2] == 'u' && content1[i+3] == 'o' && content1[i+4] == 't' && content1[i+5] == ';') 
				content1.replace(i, 6, "\"");
		}
		
		parseXML(Result);
		//printf("Result is %s\n", Result.c_str());
	}
	
	
	
	pthread_mutex_lock( &mutex2 );
	if (downloadThreadActive == true)
		getAlbumArt = false;
	pthread_mutex_unlock( &mutex2 );
	
	if (getAlbumArt == true)  {
		
		for (unsigned int i = 0; i < browseResponses.size(); i++)
		{
			if (browseResponses.at(i).isDirectory == false)  {
				
				artworkDownload.filename = browseResponses.at(i).albumArtFilename.c_str();
				
				if (access( artworkDownload.filename.c_str(), F_OK ) == -1 ) {
					//browseResponses.at(i).albumArtDownload = true;
					artworkDownload.URI = browseResponses.at(i).albumArtURI.c_str();
					artworkDownloadStack.push_back(artworkDownload);
				} 					
				
			}
		}
	
		if (artworkDownloadStack.empty() == false)  {
			//printf("vector full of artwork\n");
		
			if( (rc1=pthread_create( &thread2, NULL, DownloadArtwork, NULL)) )  {
				printf("function D - Thread creation failed: %d\n", rc1);
			} else
				downloadThreadActive = true;
		}
	}
}

/* this should return something to say we need to change mode */

bool CPlum::DirectoryUp()
{
	directoryStack.pop_back();
	
	if (directoryStack.empty() == false)  {
		currentDirectory = directoryStack.back();
		if (directoryStack.back().objectID == "-1")  {
			Reset();
			return false;
		} else
			return true;
	} else
		return false;
}

bool CPlum::DirectoryDown(string id)
{
	//printf("id is %s\n", id.c_str());
	
	for (unsigned int i = 0; i < browseResponses.size(); i++)  {
		//printf("browse id is %s %s\n", browseResponses.at(i).id.c_str(), browseResponses.at(i).title.c_str());
		if (browseResponses.at(i).id == id)  {
			browseResponses.at(i).isSelected = true;
			currentBrowseResponse = browseResponses.at(i);
		}
		else
			browseResponses.at(i).isSelected= false;
	}
	
	//printf("selected is is %s, %d\n", currentBrowseResponse.title.c_str(), currentBrowseResponse.isDirectory);
	
	if (currentBrowseResponse.isDirectory == true)  {
		//printf("dir is %s\n", currentBrowseResponse.title.c_str());
		currentDirectory.filePath += " / " + currentBrowseResponse.title;
		currentDirectory.objectID = currentBrowseResponse.id;
		currentDirectory.requestedCount = currentBrowseResponse.childCount;
		if (PMS_hack == true)  {
			if (currentDirectory.requestedCount == "1")
				currentDirectory.requestedCount = "1000";
		}
		if (currentDirectory.requestedCount == "")
			currentDirectory.requestedCount = "1000";
		directoryStack.push_back(currentDirectory);
		return false;
	} else
		return true;
}

string CPlum::GetFilePath()
{
	if (directoryStack.empty() == false)
		return directoryStack.back().filePath;
	else
		return "wrong mode";
}

bool CPlum::IsPlaylist()
{
	if (isPlaylist == true)
		return true;
	else
		return false;
}

bool CPlum::CreatePlaylist()
{
	isPlaylist = true;
	unsigned int files = 0;
	
	FILE *fp = fopen("home/playlist.pls", "w");
	if (fp)  {
		fprintf(fp, "[playlist]\n");
		for (unsigned int i = 0; i < browseResponses.size(); i++)
			if (browseResponses.at(i).isDirectory == false)  {
				fprintf(fp, "File%d=%s\n",  files+1, browseResponses.at(i).url.c_str());
				fprintf(fp, "Title%d=%s\n",  files+1, browseResponses.at(i).title.c_str());
				fprintf(fp, "Length%d=-1\n",  files+1);
				files++;
			}
		fprintf(fp, "NumberOfEntries=%d\n", files);
		fprintf(fp, "Version=2\n");
		fclose(fp);
	}
	
	if (files > 0)
		return true;
	else
		return false;
}

/*void CPlum::DownloadPreview(string url)
{
	CURL *curl;
    FILE *fp;
    CURLcode res;
    string outfilename;
    
    for (unsigned int i = 0; i < browseResponses.size(); i++)  {
		if (browseResponses.at(i).url == url)  {
				//printf("here.  album art download: %s\n", browseResponses.at(i).albumArtURI.c_str());
				outfilename = "previews/" + browseResponses.at(i).title + ".jpg";
				curl = curl_easy_init();
				if (curl) {
					fp = fopen(outfilename.c_str(),"wb");
					curl_easy_setopt(curl, CURLOPT_URL, browseResponses.at(i).albumArtURI.c_str());
					curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_preview);
					curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
					res = curl_easy_perform(curl);
					if(res != CURLE_OK) 
						fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
					curl_easy_cleanup(curl);
					fclose(fp);
				}
		}
	}
}*/




// don't pass content

void CPlum::parseXML(string content) {
  
	xmlDocPtr doc;
	xmlNode* root = NULL;
  
	// Parse the document
	doc = xmlReadMemory(content.c_str(), content.length(), "_.xml", NULL, 0);
	root = xmlDocGetRootElement(doc);
  
	// Check to make sure the document actually contains something
	if (root == NULL) {
		cout << "Document is Empty" << endl;
		xmlFreeDoc(doc);
		return;
	}
	
	
	
	//currentBrowseResponse.title = "-1";
	
	
	
	print_element_names(root);
	
	xmlFreeDoc(doc);
	xmlCleanupParser();
}





void CPlum::print_element_names(xmlNode * a_node)
{
	xmlNode *cur_node = NULL;
	xmlAttr *properties_node = NULL;

    for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
        if (cur_node->type == XML_ELEMENT_NODE) {
            //printf("node is ->%s<-\n", cur_node->name); 
            if (!xmlStrcmp(cur_node->name, (const xmlChar*) "UDN"))
				getNode = GET_UDN;
			if (!xmlStrcmp(cur_node->name, (const xmlChar*) "friendlyName"))
				getNode = GET_FRIENDLY_NAME;
			if (!xmlStrcmp(cur_node->name, (const xmlChar*) "serviceType"))
				getNode = GET_SERVICE_TYPE;
			if (getNode == GET_CONTENT_DIRECTORY && !xmlStrcmp(cur_node->name, (const xmlChar*) "controlURL"))
				getNode = GET_CONTROL_URL;
				
			if (!xmlStrcmp(cur_node->name, (const xmlChar*) "Result"))
				getNode = GET_BROWSE_RESULT;
				
			if (!xmlStrcmp(cur_node->name, (const xmlChar*) "container"))  {
				getNode = GET_CONTAINER;
				currentBrowseResponse.isDirectory = true;
				currentBrowseResponse.url = "";
				properties_node = cur_node->properties;
				while (properties_node != NULL)  {
					if (!xmlStrcmp(properties_node->name, (const xmlChar*) "id"))
						currentBrowseResponse.id = (char*)properties_node->children->content;
					if (!xmlStrcmp(properties_node->name, (const xmlChar*) "childCount"))
						currentBrowseResponse.childCount = (char*)properties_node->children->content;
					properties_node = properties_node->next;
				}
				//printf("found container id %s, childcount %s\n", currentBrowseResponse.id.c_str(), currentBrowseResponse.childCount.c_str()); 
			}
			if (getNode == GET_CONTAINER && !xmlStrcmp(cur_node->name, (const xmlChar*) "title"))
				getNode = GET_CONTAINER_TITLE;
			
			if (!xmlStrcmp(cur_node->name, (const xmlChar*) "item"))  {
				getNode = GET_ITEM;
				currentBrowseResponse.isDirectory = false;
				//currentBrowseResponse.albumArtDownload = false;
				
				properties_node = cur_node->properties;
				while (properties_node != NULL)  {
					if (!xmlStrcmp(properties_node->name, (const xmlChar*) "id"))
						currentBrowseResponse.id = (char*)properties_node->children->content;
					currentBrowseResponse.childCount = "0";
					properties_node = properties_node->next;
				}
				browseResponses.push_back(currentBrowseResponse);
			}
			
			if (getNode == GET_ITEM && !xmlStrcmp(cur_node->name, (const xmlChar*) "title"))
				getNode = GET_ITEM_TITLE;
			if (getNode == GET_ITEM && !xmlStrcmp(cur_node->name, (const xmlChar*) "res"))  {
				properties_node = cur_node->properties;
				while (properties_node != NULL)  {
					if ((!xmlStrcmp(properties_node->name, (const xmlChar*) "protocolInfo")) && ( (!xmlStrncmp(properties_node->children->content, (const xmlChar*) "http-get:*:video", 16)) || (!xmlStrncmp(properties_node->children->content, (const xmlChar*) "http-get:*:audio", 16)) ) )  {
						getNode = GET_RES;
						//printf("get res set\n");
					}
						
					properties_node = properties_node->next;
				}
				
			}
			
			if (getNode == GET_ITEM && !xmlStrcmp(cur_node->name, (const xmlChar*) "class"))
				getNode = GET_CLASS;
			
			
			if (getNode == GET_ITEM && !xmlStrcmp(cur_node->name, (const xmlChar*) "albumArtURI"))  {
				getNode = GET_ARTWORK;
				//printf("in here AA\n");
				//properties_node = cur_node->properties;
				//while (properties_node != NULL)  {
					//printf("in here BB %s\n", properties_node->name);
					//if ((!xmlStrcmp(properties_node->name, (const xmlChar*) "profileID")) && (!xmlStrcmp(properties_node->children->content, (const xmlChar*) "JPEG_TN")))
						//getNode = GET_ARTWORK;
				//	properties_node = properties_node->next;
				//}
				
			}
				
			
		}
        
        if (getNode == GET_UDN && cur_node->type == XML_TEXT_NODE) {
			currentMediaServer.UDN = (char*) cur_node->content;
            getNode = GET_NOTHING;
        }
        
        if (getNode == GET_FRIENDLY_NAME && cur_node->type == XML_TEXT_NODE) {
			currentMediaServer.friendlyName = (char*) cur_node->content;
            getNode = GET_NOTHING;
        }
        
        if (getNode == GET_SERVICE_TYPE && cur_node->type == XML_TEXT_NODE) {
			if (!xmlStrcmp(cur_node->content, (const xmlChar*) "urn:schemas-upnp-org:service:ContentDirectory:1")) 
				getNode = GET_CONTENT_DIRECTORY;
			else
				getNode = GET_NOTHING;
        }
        
        if (getNode == GET_CONTROL_URL && cur_node->type == XML_TEXT_NODE) {
			currentMediaServer.controlURL = (char*) cur_node->content;
            getNode = GET_NOTHING;
			mediaServers.push_back(currentMediaServer);
        }
        
        if (getNode == GET_BROWSE_RESULT && cur_node->type == XML_TEXT_NODE) {
			Result = (char*) cur_node->content;
            getNode = GET_NOTHING;
            return;
        }
        
        
        
        if (getNode == GET_CONTAINER_TITLE && cur_node->type == XML_TEXT_NODE) {
			currentBrowseResponse.title = (char*) cur_node->content;
			currentBrowseResponse.isDirectory = true;
			//printf("container title is %s\n", currentBrowseResponse.title.c_str());
			for (unsigned int i = 0; i < currentBrowseResponse.title.length(); i++)  {
				if (currentBrowseResponse.title[i] == '&' && currentBrowseResponse.title[i+1] == 'a' && currentBrowseResponse.title[i+2] == 'p' && currentBrowseResponse.title[i+3] == 'o' && currentBrowseResponse.title[i+4] == 's' && currentBrowseResponse.title[i+5] == ';' ) 
					currentBrowseResponse.title.replace(i, 6, "'");
				if (currentBrowseResponse.title[i] == '&' && currentBrowseResponse.title[i+1] == 'a' && currentBrowseResponse.title[i+2] == 'm' && currentBrowseResponse.title[i+3] == 'p' && currentBrowseResponse.title[i+4] == ';' ) 
					currentBrowseResponse.title.replace(i, 5, "&");
			}
			
			//currentBrowseResponse.albumArtDownload = false;
			browseResponses.push_back(currentBrowseResponse);
			getNode = GET_NOTHING;
		}
		
		
		if (getNode == GET_ITEM_TITLE && cur_node->type == XML_TEXT_NODE) {
			
			browseResponses.back().title = (char*) cur_node->content;
			for (unsigned int i = 0; i < browseResponses.back().title.length(); i++)  {
				if (browseResponses.back().title[i] == '&' && browseResponses.back().title[i+1] == 'a' && browseResponses.back().title[i+2] == 'p' && browseResponses.back().title[i+3] == 'o' && browseResponses.back().title[i+4] == 's' && browseResponses.back().title[i+5] == ';' ) 
					browseResponses.back().title.replace(i, 6, "'");
				if (browseResponses.back().title[i] == '&' && browseResponses.back().title[i+1] == 'a' && browseResponses.back().title[i+2] == 'm' && browseResponses.back().title[i+3] == 'p' && browseResponses.back().title[i+4] == ';' ) 
					browseResponses.back().title.replace(i, 5, "&");
			}
			
			browseResponses.back().albumArtFilename = browseResponses.back().title.c_str();
			for (unsigned int i = 0; i < browseResponses.back().albumArtFilename.length(); i++)
				if (browseResponses.back().albumArtFilename[i] == '/' || browseResponses.back().albumArtFilename[i] == ':') 
					browseResponses.back().albumArtFilename.replace(i, 1, "_");
			browseResponses.back().albumArtFilename = previewsPath + "/" +  browseResponses.back().albumArtFilename;
			
			getNode = GET_ITEM;
		}
		
		if (getNode == GET_CLASS && cur_node->type == XML_TEXT_NODE) {
			//printf("in here %s\n", cur_node->content);
			browseResponses.back().upnpclass = (char*) cur_node->content;
			getNode = GET_ITEM;
		}
		
		if (getNode == GET_ARTWORK && cur_node->type == XML_TEXT_NODE) {
			//printf("in here %s\n", cur_node->content);
			browseResponses.back().albumArtURI = (char*) cur_node->content;
			getNode = GET_ITEM;
		}
		
		if (getNode == GET_RES && cur_node->type == XML_TEXT_NODE) {
			browseResponses.back().url = (char*) cur_node->content;
			getNode = GET_ITEM;
		}
		
		print_element_names(cur_node->children);
    }
}
