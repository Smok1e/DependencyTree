#pragma once

//---------------------

class ModuleInfo: public BasicModuleInfo
{
public:
	ModuleInfo ();
	ModuleInfo (HMODULE module);
	ModuleInfo (const ModuleInfo& copy);

	bool load (HMODULE module);

	virtual bool ok () const;

	template <typename obj_t> obj_t RVA (uintptr_t offset);

	HMODULE getModuleHandle ();

	const char* getOriginalModuleName ();
	      char* getModuleFilename     (char* buffer, size_t max);

	int                               getExportFunctionsCount      ();
	int                               getExportFunctionsNamesCount ();
	const char*                       getExportFunctionName        (int         index);
	int                               getExportFunctionIndex       (const char* name );
	template <typename proc_t> proc_t getExportFunctionAddress     (int         index);
	template <typename proc_t> proc_t getExportFunctionAddress     (const char* name );
	template <typename proc_t> bool   setExportFunctionAddress     (int         index, proc_t new_proc);
	template <typename proc_t> bool   setExportFunctionAddress     (const char* name,  proc_t new_proc);

	int                               getImportModulesCount        ();
	const char*                       getImportModuleName          (int         index);
	int                               getImportModuleIndex         (const char* name );
	int                               getImportFunctionsCount      (int module_index );
	const char*                       getImportFunctionName        (int module_index, int function_index);
	int                               getImportFunctionIndex       (int module_index, const char* name          );
	template <typename proc_t> proc_t getImportFunctionAddress     (int module_index, int         function_index);
	template <typename proc_t> proc_t getImportFunctionAddress     (int module_index, const char* name          );
	template <typename proc_t> proc_t getImportFunctionAddress     (                  const char* name          );
	template <typename proc_t> bool   setImportFunctionAddress     (int module_index, int         function_index, proc_t new_proc);
	template <typename proc_t> bool   setImportFunctionAddress     (int module_index, const char* name,           proc_t new_proc);
	template <typename proc_t> bool   setImportFunctionAddress     (                  const char* name,           proc_t new_proc);

	IMAGE_DOS_HEADER*        getDOSEntry    ();
	IMAGE_NT_HEADERS*        getNTEntry     ();
	IMAGE_EXPORT_DIRECTORY*  getExportEntry ();
	IMAGE_IMPORT_DESCRIPTOR* getImportEntry ();

protected:
	HMODULE                  m_module;
	IMAGE_NT_HEADERS*        m_nt_entry;
	IMAGE_EXPORT_DIRECTORY*  m_export_entry;
	IMAGE_IMPORT_DESCRIPTOR* m_import_entry;

};

//---------------------

ModuleInfo::ModuleInfo ():
	BasicModuleInfo (),
	m_module       (nullptr),
	m_nt_entry     (nullptr),
	m_export_entry (nullptr),
	m_import_entry (nullptr)
{}

ModuleInfo::ModuleInfo (HMODULE module):
	BasicModuleInfo (),
	m_module       (nullptr),
	m_nt_entry     (nullptr),
	m_export_entry (nullptr),
	m_import_entry (nullptr)
{
	load (module);
}

ModuleInfo::ModuleInfo (const ModuleInfo& copy):
	BasicModuleInfo (),
	m_module       (nullptr),
	m_nt_entry     (nullptr),
	m_export_entry (nullptr),
	m_import_entry (nullptr)
{
	load (copy.m_module);
}

//---------------------

bool ModuleInfo::load (HMODULE module)
{
	if (!module)
	{
		formatError ("Failed to load module info: Module is nullptr");
		return false;
	}

	m_module = module;

	IMAGE_DOS_HEADER* dos_header = RVA <IMAGE_DOS_HEADER*> (0);
	if (!dos_header) return false;

	if (dos_header -> e_magic != IMAGE_DOS_SIGNATURE) 
	{
		m_module = nullptr;
		formatError ("Failed to load module info: Wrong dos signature (0x%04X)", dos_header -> e_magic);
		return false;
	}

	m_nt_entry = RVA <IMAGE_NT_HEADERS*> (dos_header -> e_lfanew);
	if (!m_nt_entry) return false;

	if (m_nt_entry -> Signature != IMAGE_NT_SIGNATURE)
	{
		m_module = nullptr;
		formatError ("Failed to load module info: Wrong NT signature (0x%08X)", m_nt_entry -> Signature);
		return false;
	}

	m_export_entry = RVA <IMAGE_EXPORT_DIRECTORY*>  (m_nt_entry -> OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
	m_import_entry = RVA <IMAGE_IMPORT_DESCRIPTOR*>	(m_nt_entry -> OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

	if (!m_export_entry || !m_import_entry)
	{
		m_module = nullptr;
		return false;
	}

	if (hasError ())
	{
		m_module = nullptr;
		return false;
	}

	return true;
}

//---------------------

bool ModuleInfo::ok () const
{
	return !m_has_error && m_module;
}

//---------------------

template <typename obj_t>
obj_t ModuleInfo::RVA (uintptr_t offset)
{
	if (!m_module)
	{
		formatError ("Failed to get relative virtual address: Module is nullptr");
		return {};
	}

	return (obj_t) ((uintptr_t) m_module + offset);
}

//---------------------

HMODULE ModuleInfo::getModuleHandle ()
{
	return m_module;
}

//---------------------

const char* ModuleInfo::getOriginalModuleName ()
{
	return RVA <const char*> (m_export_entry -> Name);
}

//---------------------

char* ModuleInfo::getModuleFilename (char* buffer, size_t max)
{
	GetModuleFileNameA (m_module, buffer, max);
	return buffer;
}

//---------------------

int ModuleInfo::getExportFunctionsCount ()
{
	return m_export_entry -> NumberOfFunctions;
}

//---------------------

int ModuleInfo::getExportFunctionsNamesCount ()
{
	return m_export_entry -> NumberOfNames;
}

//---------------------

const char* ModuleInfo::getExportFunctionName (int index)
{
	DWORD* names_entry = RVA <DWORD*     > (m_export_entry -> AddressOfNames);
	return               RVA <const char*> (names_entry[index]              );	
}

int ModuleInfo::getExportFunctionIndex (const char* name)
{
	for (size_t i = 0, count = getExportFunctionsCount (); i < count; i++)
		if (!_stricmp (getExportFunctionName (i), name)) return i;

	return -1;
}

//---------------------

template <typename proc_t>
proc_t ModuleInfo::getExportFunctionAddress (int index)
{
	if (index < 0 || index >= getExportFunctionsCount ())
	{
		formatError ("Failed to get export function proc address: Index out of range");
		return 0;
	}

	DWORD* funcs_entry = RVA <DWORD*> (m_export_entry -> AddressOfFunctions);
	return RVA <proc_t> (funcs_entry[index]);
}

template <typename proc_t>
proc_t ModuleInfo::getExportFunctionAddress (const char* name)
{
	int index = getExportFunctionIndex (name);
	if (index == -1)
	{
		formatError ("Failed to get export function proc address: Specified procedure not found");
		return nullptr;
	}

	return getExportFunctionAddress <proc_t> (index);
}

//---------------------

template <typename proc_t>
bool ModuleInfo::setExportFunctionAddress (int index, proc_t new_proc)
{
	if (!new_proc)
	{
		formatError ("Failed to set export function proc address: New proc address was nullptr");
		return false;
	}

	if (index < 0 || index >= getExportFunctionsCount ())
	{
		formatError ("Failed to set export function proc address: Index out of range");
		return false;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////
	//    																					           //
	// 	+--- Module base + Address of functions + function index            				           //
	//  |																					           //
	//  |	 [0]: (base + address of functions + 0x00000000)								           //
	// 	|	 [1]: (base + address of functions + 0x00000004)										   //
	// 	+--> [2]: (base + address of functions + 0x00000008) <-- should change to my function pointer  //
	// 		 [3]: (base + address of functions + 0x0000000C)									       //
	//   	 [4]: (base + address of functions + 0x00000010)										   //
	//    	 [.]: (base + address of functions + ...	   )			     						   //
	//    	 [N]: (base + address of functions + N - 1     )         							       //
	//																						           //
	/////////////////////////////////////////////////////////////////////////////////////////////////////
	
	uintptr_t  base_address      = (uintptr_t ) m_module;
	uintptr_t  funcs_address     = (uintptr_t ) m_export_entry -> AddressOfFunctions;
	uintptr_t* offset            = (uintptr_t*) (base_address + funcs_address + index * sizeof (DWORD));
	uintptr_t  relative_new_proc = (uintptr_t ) new_proc - base_address;

	DWORD rights = PAGE_READWRITE;
	if (!VirtualProtect (offset, sizeof (*offset), rights, &rights))
	{
		static char errmsg[BUFFSIZE] = "";
		formatError ("Failed to set export function address: %s\n", FormatWinapiError (errmsg, BUFFSIZE, GetLastError ()));
		return false;
	}

	std::memcpy (offset, &relative_new_proc, sizeof (*offset));

	VirtualProtect (offset, sizeof (*offset), rights, &rights);
	return true;
}

//---------------------

template <typename proc_t>
bool ModuleInfo::setExportFunctionAddress (const char* name, proc_t new_proc)
{
	int index = getExportFunctionIndex (name);
	if (index == -1)
	{
		formatError ("Failed to set export function proc address: Specified procedure not found");
		return false;
	}

	return setExportFunctionAddress (index, new_proc);
}

/*
 
 	      IMAGE_DOS_HEADER:
          +============================+
          ║ WORD e_magic // 0x5A4D     ║
          ║ LONG e_lfanew -------------╬-----------------------+ 
          ║ ...			               ║					   |
          +============================+					   |
          													   |
          IMAGE_NT_HEADERS:          				   		   |
          +==============================================+ <---+
          ║ DWORD Signature // 0x50450000				 ║
          ║ IMAGE_FILE_HEADER FileHeader				 ║
          ║ +=========================================+  ║
          ║ ║ IMAGE_OPTIONAL_HEADER OptionalHeader	  ║  ║
          ║ ║ ├─WORD Magic 						      ║  ║
          ║ ║ ├─IMAGE_DATA_DIRECTORY DataDirectory[] -╬--╬---------+
          ║ ║ └─...								      ║  ║         |
          ║ +=========================================+  ║         |
          ║ ...										     ║         |
          +==============================================+         |
      													           |
 	      IMAGE_DATA_DIRECTORY[IMAGE_DIRECTORY_ENTRY_IMPORT]:	   |
 	      +==================================================+ <---+
 	+-----╬ DWORD VirtualAddress                             ║
    |     ║ DWORD Size			                             ║                       
    |     +==================================================+
    |
    |     IMAGE_IMPORT_DESCRIPTOR (DLL 0):
    +---> +================================+
	|	  ║ DWORD OriginalFirstThunk ------╬------------------------------------+
	|	  ║ DWORD Name					   ║									|
 	|	  ║ DWORD FirstThunk --------------╬---+								|
	|	  ║ ...							   ║   |	 IMAGE_THUNK_DATA:			|	  IMAGE_THUNK_DATA:
    |     +================================+   +---> +=====================+    +---> +=========================================+
	|												 ║ PDWORD Function	 0 ║		  ║ PIMAGE_IMPORT_BY_NAME AddressOfData   0 ║
    |     IMAGE_IMPORT_DESCRIPTOR (DLL 1):			 ║ ...				   ║		  ║	...										║
    +---> +================================+		 ╠---------------------╣		  ╠-----------------------------------------╣
	|	  ║ DWORD OriginalFirstThunk	   ║	 +---╬ PDWORD Function	 1 ║	  +---╬ PIMAGE_IMPORT_BY_NAME AddressOfData   1 ║
	|	  ║ DWORD Name					   ║     |   ║ ...				   ║      |   ║	...										║
 	|	  ║ DWORD FirstThunk			   ║     |   ╠---------------------╣      |   ╠-----------------------------------------╣
	|	  ║ ...							   ║     |   ║ PDWORD Function	 2 ║      |   ║ PIMAGE_IMPORT_BY_NAME AddressOfData   2 ║
    |     +================================+     |   ║ ...				   ║      |   ║	...										║
	|										     |   ╠---------------------╣      |   ╠-----------------------------------------╣
    |     IMAGE_IMPORT_DESCRIPTOR (DLL 2):	     |   ║ PDWORD Function	 N ║      |   ║ PIMAGE_IMPORT_BY_NAME AddressOfData   N ║
    +---> +================================+     |   ║ ...				   ║      |   ║	...										║
	|	  ║ DWORD OriginalFirstThunk	   ║     |   +=====================+      |   +=========================================+
	|	  ║ DWORD Name					   ║	 |								  |
 	|	  ║ DWORD FirstThunk			   ║	 |								  +------>> Function name
	|	  ║ ...							   ║	 +--------------------------------------->> Function pointer
    |      +================================+
	|
    |      IMAGE_IMPORT_DESCRIPTOR (DLL N):	
    +---> +================================+
		  ║ DWORD OriginalFirstThunk	   ║
		  ║ DWORD Name					   ║
 		  ║ DWORD FirstThunk			   ║
		  ║ ...							   ║
          +================================+

*/

int ModuleInfo::getImportModulesCount ()
{
	int count = 0;
	for (IMAGE_IMPORT_DESCRIPTOR* desc = m_import_entry; desc -> Name; desc++, count++);

	return count;
}
 
//---------------------

const char* ModuleInfo::getImportModuleName (int index)
{
	if (index < 0 || index >= getImportModulesCount ())
	{
		formatError ("Failed to get import module name: Index out of range");
		return nullptr;
	}

	IMAGE_IMPORT_DESCRIPTOR* desc = m_import_entry + index;
	return RVA <const char*> (desc -> Name);
}

//---------------------

int ModuleInfo::getImportModuleIndex (const char* name)
{
	for (size_t i = 0, count = getImportModulesCount (); i < count; i++)
		if (!_stricmp (getImportModuleName (i), name)) return i;

	return -1;
}

//---------------------

int ModuleInfo::getImportFunctionsCount (int module_index)
{
	if (module_index < 0 || module_index >= getImportModulesCount ())
	{
		formatError ("Failed to get import functions count: Index out of range");
		return -1;
	}

	IMAGE_IMPORT_DESCRIPTOR* desc = m_import_entry + module_index;

	int count = 0;
	for (IMAGE_THUNK_DATA* thunk = RVA <IMAGE_THUNK_DATA*> (desc -> FirstThunk); thunk -> u1.Function; thunk++, count++);

	return count;
}

//---------------------

const char* ModuleInfo::getImportFunctionName (int module_index, int function_index)
{
	if (module_index < 0 || module_index >= getImportModulesCount ())
	{
		formatError ("Failed to get import function name: Module index out of range");
		return nullptr;
	}

	if (function_index < 0 || function_index >= getImportFunctionsCount (module_index))
	{
		formatError ("Failed to get import function name: Function index out of range");
		return nullptr;
	}

	IMAGE_IMPORT_DESCRIPTOR* desc  = m_import_entry + module_index;
	IMAGE_THUNK_DATA*        thunk = RVA <IMAGE_THUNK_DATA*>     (desc  -> OriginalFirstThunk + function_index * sizeof (IMAGE_THUNK_DATA));
	IMAGE_IMPORT_BY_NAME*    name  = RVA <IMAGE_IMPORT_BY_NAME*> (thunk -> u1.AddressOfData                                               );
	return name -> Name;
}

//---------------------

int ModuleInfo::getImportFunctionIndex (int module_index, const char* name)
{
	if (module_index < 0 || module_index >= getImportModulesCount ())
	{
		formatError ("Failed to get import function index: Module index out of range");
		return -1;
	}

	for (size_t i = 0, count = getImportFunctionsCount (module_index); i < count; i++)
		if (!_stricmp (getImportFunctionName (module_index, i), name)) return i;

	return -1;
}

//---------------------

template <typename proc_t> 
proc_t ModuleInfo::getImportFunctionAddress (int module_index, int function_index)
{
	if (module_index < 0 || module_index >= getImportModulesCount ())
	{
		formatError ("Failed to get import function address: Module index out of range");
		return nullptr;
	}

	if (function_index < 0 || function_index >= getImportFunctionsCount (module_index))
	{
		formatError ("Failed to get import function address: Function index out of range");
		return nullptr;
	}

	IMAGE_IMPORT_DESCRIPTOR* desc  = m_import_entry + module_index;
	IMAGE_THUNK_DATA*        thunk = RVA <IMAGE_THUNK_DATA*> (desc  -> FirstThunk + function_index * sizeof (IMAGE_THUNK_DATA));
	return reinterpret_cast <proc_t> (thunk -> u1.Function);
}

//---------------------

template <typename proc_t> 
proc_t ModuleInfo::getImportFunctionAddress (int module_index, const char* name)
{
	if (module_index < 0 || module_index >= getImportModulesCount ())
	{
		formatError ("Failed to get import function address: Module index out of range");
		return nullptr;
	}

	int function_index = getImportFunctionIndex (module_index, name);
	if (function_index == -1)
	{
		formatError ("Failed to get import function address: Specified procedure not found");
		return nullptr;
	}

	return getImportFunctionAddress <proc_t> (module_index, function_index);
}

//---------------------

template <typename proc_t>
proc_t ModuleInfo::getImportFunctionAddress (const char* name)
{
	for (size_t module_index = 0, modules_count = getImportModulesCount (); module_index < modules_count; module_index++)
		for (size_t func_index = 0, funcs_count = getImportFunctionsCount (module_index); func_index < funcs_count; func_index++)
			if (!_stricmp (getImportFunctionName (module_index, func_index), name)) return getImportFunctionAddress <proc_t> (module_index, func_index);

	formatError ("Failed to get import function index: Specified procedure not found");
	return nullptr;
}

//---------------------

template <typename proc_t>
bool ModuleInfo::setImportFunctionAddress (int module_index, int function_index, proc_t new_proc)
{
	if (module_index < 0 || module_index >= getImportModulesCount ())
	{
		formatError ("Failed to set import function address: Module index out of range");
		return false;
	}

	if (function_index < 0 || function_index >= getImportFunctionsCount (module_index))
	{
		formatError ("Failed to set import function address: Function index out of range");
		return false;
	}

	if (!new_proc)
	{
		printf ("Failed to set import function address: New address was nullptr");
		return false;
	}

	IMAGE_IMPORT_DESCRIPTOR* desc  = m_import_entry + module_index;
	IMAGE_THUNK_DATA*        thunk = RVA <IMAGE_THUNK_DATA*> (desc  -> FirstThunk + function_index * sizeof (IMAGE_THUNK_DATA));

	uintptr_t* func = (uintptr_t*) &(thunk -> u1.Function);

	DWORD rights = PAGE_READWRITE;
	if (!VirtualProtect (func, sizeof (*func), rights, &rights))
	{
		static char errmsg[BUFFSIZE] = "";
		formatError ("Failed to get import function address: %s", FormatWinapiError (errmsg, BUFFSIZE, GetLastError ()));
		return false;
	}

	std::memcpy (func, &new_proc, sizeof (*func));

	VirtualProtect (func, sizeof (*func), rights, &rights);
	return true;
}

//---------------------

template <typename proc_t>
bool ModuleInfo::setImportFunctionAddress (int module_index, const char* name, proc_t new_proc)
{
	if (module_index < 0 || module_index >= getImportModulesCount ())
	{
		formatError ("Failed to set import function address: Module index out of range");
		return nullptr;
	}

	int function_index = getImportFunctionIndex (module_index, name);
	if (function_index == -1)
	{
		formatError ("Failed to set import function address: Specified procedure not found");
		return nullptr;
	}

	return setImportFunctionAddress <proc_t> (module_index, function_index, new_proc);	
}

//---------------------

template <typename proc_t>
bool ModuleInfo::setImportFunctionAddress (const char* name, proc_t new_proc)
{
	for (size_t module_index = 0, modules_count = getImportModulesCount (); module_index < modules_count; module_index++)
		for (size_t func_index = 0, funcs_count = getImportFunctionsCount (module_index); func_index < funcs_count; func_index++)
			if (!_stricmp (getImportFunctionName (module_index, func_index), name)) return setImportFunctionAddress <proc_t> (module_index, func_index, new_proc);

	formatError ("Failed to set import function index: Specified procedure not found");
	return false;	
}

//---------------------

IMAGE_DOS_HEADER* ModuleInfo::getDOSEntry ()
{
	return RVA <IMAGE_DOS_HEADER*> (0);
}

IMAGE_NT_HEADERS* ModuleInfo::getNTEntry ()
{
	return m_nt_entry;
}

IMAGE_EXPORT_DIRECTORY* ModuleInfo::getExportEntry ()
{
	return m_export_entry;
}

IMAGE_IMPORT_DESCRIPTOR* ModuleInfo::getImportEntry ()
{
	return m_import_entry;
}

//---------------------