#include <cstdio>
#include <memory>
#include <vector>
#include <string>

#include <Windows.h>
#include "BasicModuleInfo.h"
#include "ModuleInfo.h"
#include "Graph.h"

//------------------------

#define RECURSION_LIMIT 32

//------------------------

bool DumpDependencies (Graph* graph, const char* dllname, const char* parent = nullptr, int recursion = 0);

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

	DumpDependencies (&graph, "notepad.exe");

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

bool DumpDependencies (Graph* graph, const char* dllname, const char* parent /*= nullptr*/, int recursion /*= 0*/)
{
	if (recursion >= RECURSION_LIMIT)
	{
		printf ("Warning: Recursion calls limit exceeded (%d)\n", recursion);
		return false;
	}

	if (parent && (dllname == parent || strcmp (dllname, parent) == 0))
	{
		graph -> add ("\"%s\" [style = filled, fillcolor = \"#464513\", label = \"%s\"];", dllname, dllname);
		graph -> add ("\"%s\" -> \"%s\" [color = \"#FFFF00\", fillcolor = \"#FFFF00\"];", dllname, parent);
		return false;
	}

	HMODULE module = LoadLibraryA (dllname);
	if (!module)
	{
		graph -> add ("\"%s\" [style = filled, fillcolor = \"#5E1B1B\", label = \"%s\"];", dllname, dllname);
		if (parent)
			graph -> add ("\"%s\" -> \"%s\" [color = \"#5e1b1b\", fillcolor = \"#FF0000\"];", parent, dllname);

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

	if (parent)
		graph -> add ("\"%s\" -> \"%s\"", parent, dllname);

	else graph -> add ("\"%s\" [style = filled, fillcolor = \"#12304D\"]", dllname);

	if (_stricmp (dllname, "NTDLL.DLL") == 0)
	{
		graph -> add ("\"%s\" [style = filled, fillcolor = \"#13463C\"]", dllname);

		FreeLibrary (module);
		return true;
	}

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

		if (!DumpDependencies (graph, info.getImportModuleName (i), dllname, recursion+1))
		{
			FreeLibrary (module);
			return false;
		}
	}

	FreeLibrary (module);
	return true;
}

//------------------------