#include <cstdio>
#include <memory>
#include <vector>
#include <string>

#include <Windows.h>
#include "BasicModuleInfo.h"
#include "ModuleInfo.h"
#include "Graph.h"

//------------------------

bool DumpDependencies (Graph* graph, const char* dllname, const char* parent = nullptr, int indent = 0);

//------------------------

int main ()
{
	Graph graph ("dependencies");
	graph.add ("dpi = 200;");
	graph.add ("bgcolor = \"#181818\"");
	graph.add ("splines = ortho;");
	graph.add ("ranksep = 2;");
	graph.add ("");
	graph.add ("node [shape = signature, color = white, fontcolor = white, fontname = consolas]");
	graph.add ("edge [color = white, fillcolor = white]");
	graph.add ("");

	DumpDependencies (&graph, "C:\\Program Files (x86)\\Far Manager\\Far.exe");

	int result = graph.render ();
	if (result == 0)
	{
		char cmd[BUFFSIZE] = "";
		sprintf_s (cmd, "start %s", graph.image.c_str ());
		system (cmd);
	}

	else printf ("dot.exe exited with error 0x%08X (%d)\n", result, result);
}

//------------------------

bool DumpDependencies (Graph* graph, const char* dllname, const char* parent /*= nullptr*/, int indent /*= 0*/)
{
	HMODULE module = LoadLibraryA (dllname);
	if (!module)
	{
		printf ("Warning: Failed to load library '%s': %s\n", dllname, FormatWinapiError (GetLastError ()));
		return true;
	}
												
	ModuleInfo info;
	if (!info.load (module))
	{
		FreeLibrary (module);

		printf ("%s\n", info.getError ());
		return false;
	}

	const char* name = info.getOriginalModuleName ();
	if 
	(
		_stricmp (name, "NTDLL.DLL"     ) == 0 || // эти дллки по какой то причине образуют кольцевую связь и как следствие переполнение стека
		_stricmp (name, "KERNELBASE.DLL") == 0 ||
		_stricmp (name, "KERNEL32.DLL"  ) == 0 ||
		_stricmp (name, "UCRTBASE.dll"  ) == 0 ||
		_stricmp (name, "MSVCP_WIN.dll" ) == 0 ||
		_stricmp (name, "GDI32FULL.dll" ) == 0
	)
	{
		FreeLibrary (module);
		return true;
	}

	if (parent)
		graph -> add ("\"%s\" -> \"%s\"", parent, name);

	//printf ("%*s'%s':\n", indent*2, "", info.getOriginalModuleName ());

	int count = info.getImportModulesCount ();
	for (size_t i = 0; i < count; i++)
	{
		for (size_t j = 0; j < i; j++)
		{
			if (strcmp (info.getImportModuleName (j), info.getImportModuleName (i)) == 0)
			{
				FreeLibrary (module);
				return true;
			}
		}

		if (!DumpDependencies (graph, info.getImportModuleName (i), dllname, indent+1))
		{
			FreeLibrary (module);
			return false;
		}
	}

	FreeLibrary (module);
	return true;
}

//------------------------