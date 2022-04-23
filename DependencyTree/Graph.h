#pragma once

//--------------------------------

#include <string>
#include <cstdio>
#include <cassert>
#include <stdarg.h>

//--------------------------------

#ifdef API_EXPORT
	#undef API_EXPORT
#endif
#define API_EXPORT __declspec (dllexport)

#ifdef API_IMPORT
	#undef API_IMPORT
#endif
#define API_IMPORT __declspec (dllimport)

#ifdef EXPORTING
	#define DECLSPEC API_EXPORT
#else
	#define DECLSPEC API_IMPORT
#endif

//--------------------------------

#ifdef BUFFSIZE
	#undef BUFFSIZE
#endif
#define BUFFSIZE 512

#ifndef DOT_PATH
	#define DOT_PATH "C:/Program Files/Graphviz/bin/dot.exe"
#endif

//--------------------------------

class DECLSPEC Graph
{
public :
	 Graph (std::string name);
	~Graph ();

	void add       (const char* format, ...);
	int  render    ();
	bool available ();
	
	std::string getFilename ();
	__declspec (property (get = getFilename)) std::string filename;

	std::string getImage ();
	__declspec (property (get = getImage)) std::string image;

	struct DECLSPEC Color
	{
		static const char* White;
		static const char* Green;
		static const char* LightGreen;
		static const char* Blue;
		static const char* LightBlue;
		static const char* DarkGrey;
		static const char* Transparent;
	};

private :
	FILE*       m_file;
	std::string m_name;
	int         m_tabs;

};

//--------------------------------