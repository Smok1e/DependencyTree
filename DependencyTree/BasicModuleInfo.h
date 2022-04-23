#pragma once

#ifdef BUFFSIZE
	#undef BUFFSIZE
#endif
#define BUFFSIZE 4096

//---------------------

      char* FormatWinapiError (char* errbuff, size_t max, int err);
const char* FormatWinapiError (                           int err);

//---------------------

class BasicModuleInfo
{
public:
	BasicModuleInfo ();

	const char* getError   () const;
	bool        hasError   () const;
	void        clearError ();

	virtual bool ok () const;

protected:
	char m_errbuff[BUFFSIZE] = "";
	bool m_has_error;

	void formatError (const char* format, ...);

};

//---------------------

BasicModuleInfo::BasicModuleInfo ():
	m_errbuff   (""),
	m_has_error (false)
{}

//---------------------

const char* BasicModuleInfo::getError () const
{
	return m_has_error? m_errbuff: "No error";
}

bool BasicModuleInfo::hasError () const
{
	return m_has_error;
}

void BasicModuleInfo::clearError ()
{
	m_has_error = false;
}

//---------------------

bool BasicModuleInfo::ok () const
{
	return !m_has_error;
}

//---------------------

void BasicModuleInfo::formatError (const char* format, ...)
{
	va_list args = {};
	va_start (args, format);
	vsprintf_s (m_errbuff, format, args);
	va_end (args);

	#ifdef MODULE_ERROR_OUTPUT
		printf ("[DEBUG] Module info error: %s\n", m_errbuff);
	#endif

	m_has_error = true;
}

//---------------------

char* FormatWinapiError (char* errbuff, size_t max, int err)
{
	const char* remove = "\n\r.";

	char* tmp = new char[max];
	FormatMessageA (FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, err, MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT), tmp, max, nullptr);

	size_t c = 0;
	for (size_t i = 0; i < max-1 && tmp[i]; i++)
		if (!strchr (remove, tmp[i])) errbuff[c++] = tmp[i];
	errbuff[c] = '\0';

	delete[] (tmp);
	return errbuff;
}

const char* FormatWinapiError (int err)
{
	static char errbuff[BUFFSIZE] = "";
	return FormatWinapiError (errbuff, BUFFSIZE, err);
}

//---------------------