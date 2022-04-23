#define EXPORTING

//--------------------------------

#include "Graph.h"

//--------------------------------

const char* Graph::Color::White       = "\"#FFFFFFFF\"";
const char* Graph::Color::Green       = "\"#257D22FF\"";
const char* Graph::Color::LightGreen  = "\"#D0FFD0FF\"";
const char* Graph::Color::Blue        = "\"#0080FFFF\"";
const char* Graph::Color::LightBlue   = "\"#9ED7FFFF\"";
const char* Graph::Color::DarkGrey    = "\"#181818FF\"";
const char* Graph::Color::Transparent = "\"#00000000\"";

//--------------------------------

Graph::Graph (std::string name):
	m_name (name),
	m_file (nullptr),
	m_tabs (0)
{
	fopen_s (&m_file, filename.c_str (), "w");
	assert (m_file);

	add ("strict digraph G");
	add ("{");
}

Graph::~Graph ()
{
	fclose (m_file);
	m_file = nullptr;
}

//--------------------------------

void Graph::add (const char* format, ...)
{
	va_list args = {};
	va_start (args, format);
	size_t buffsize = vsnprintf (nullptr, 0, format, args) + 1;
	va_end (args);

	char* str = new char[buffsize];

	va_start (args, format);
	vsprintf_s (str, buffsize, format, args);
	va_end (args);

	size_t indent = 4*m_tabs;
	for (const char* l_str = str; *l_str; l_str++)
	{
		m_tabs += *l_str == '{';

		if (*l_str == '}')
		{
			m_tabs--;
			indent = 4*m_tabs;
		}
	}

	if (m_tabs < 0)
		m_tabs = 0;

	fprintf (m_file, "%*s%s\n", indent, "", str);
	delete[] (str);
}

//--------------------------------

int Graph::render ()
{
	add ("}");

	assert (available ());

	char cmd [BUFFSIZE] = "";
	sprintf_s (cmd, "\"%s\" %s -Tpng -o%s", DOT_PATH, filename.c_str (), image.c_str ());

	fclose (m_file);

	int status = system (cmd);
	if (status == 0)
		fopen_s (&m_file, filename.c_str (), "a");

	return status;
}

//--------------------------------

std::string Graph::getFilename ()
{
	return m_name + ".graph.txt";
}

std::string Graph::getImage ()
{
	return m_name + ".png";
}

//--------------------------------

bool Graph::available ()
{
	FILE* file = nullptr;
	errno_t err = fopen_s (&file, DOT_PATH, "r");
	if (!file || err)
	{
		if (file) fclose (file);
		return false;
	}

	fclose (file);
	return true;
}

//--------------------------------