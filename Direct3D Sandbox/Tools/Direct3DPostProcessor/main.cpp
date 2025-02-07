#include "PrecompiledHeader.h"
#include "..\..\Source\Core\Tools.h"
#include "ManagedInvoker.h"
#include "ModelProcessor.h"
#include "ShaderReflector.h"

#include <comutil.h>

static void ProcessShaders(wstring shaderDirectory)
{
	for (auto& shaderPath : Tools::GetFilesInDirectory(shaderDirectory, L"*.cso", true))
	{
		wcout << "Reflecting on shader: " << shaderPath << endl;
		ShaderReflector::ReflectShader(shaderPath);
	}
}

static void ProcessModels(wstring modelInputDirectory, wstring modelOutputDirectory)
{
	if (!Tools::DirectoryExists(modelInputDirectory))
	{
		wcout << "ERROR: Could not find animated models input directory: \"" << modelInputDirectory << "\"." << endl;
		exit(-1);
	}

	if (!Tools::DirectoryExists(modelOutputDirectory))
	{
		CreateDirectory(modelOutputDirectory.c_str(), nullptr);
	}
	
	wcout << endl;
	for (auto& modelPath : Tools::GetFilesInDirectory(modelInputDirectory, L"*.obj", true))
	{
		wcout << L"Processing model: " << modelPath << endl;
		ModelProcessor::ProcessModel(modelPath, modelOutputDirectory);
	}
}

static void ProcessAnimatedModels(wstring modelInputDirectory, wstring modelOutputDirectory)
{
	if (!Tools::DirectoryExists(modelInputDirectory))
	{
		wcout << "ERROR: Could not find animated models input directory: \"" << modelInputDirectory << "\"." << endl;
		exit(-1);
	}

	if (!Tools::DirectoryExists(modelOutputDirectory))
	{
		CreateDirectory(modelOutputDirectory.c_str(), nullptr);
	}

	wcout << endl;
	for (auto& animatedModelDirectory : Tools::GetDirectories(modelInputDirectory, false))
	{
		wcout << L"Processing animated model: " << animatedModelDirectory << endl << endl;
		ModelProcessor::ProcessAnimatedModel(animatedModelDirectory, modelOutputDirectory);
	}
}

static void PutItem(SAFEARRAY* safeArray, LONG index, _variant_t item)
{
	auto result = SafeArrayPutElement(safeArray, &index, &item);
	Assert(result == S_OK);
}

static wstring GetExecutableDir()
{	
	wchar_t pathBuffer[MAX_PATH];
	auto result = GetModuleFileName(nullptr, pathBuffer, MAX_PATH);
	Assert(result != 0);

	wstring path = pathBuffer;

	return path.substr(0, path.find_last_of(L"\\") + 1);
}

static void ProcessFont(const wstring& fontName, float fontSize, const wstring& outputDirectory)
{	
	HRESULT result;
	ManagedInvoker invoker;

	wcout << "Processing font: " << fontName << ", size " << fontSize << "." << endl;

	if (!Tools::DirectoryExists(outputDirectory))
	{
		CreateDirectory(outputDirectory.c_str(), nullptr);
	}

	auto arguments = SafeArrayCreateVector(VT_VARIANT, 0, 3);
	Assert(arguments != nullptr);
	
	PutItem(arguments, 0, fontName.c_str());
	PutItem(arguments, 1, fontSize);
	PutItem(arguments, 2, outputDirectory.c_str());
	
	invoker.Execute(GetExecutableDir() + L"ManagedPostProcessor.dll", L"ManagedPostProcessor.FontCreator", L"CreateFont", arguments);
	
	result = SafeArrayDestroy(arguments);
	Assert(result == S_OK);

	wcout << endl;
}

static void ProcessFonts(const wstring& fontOutputDirectory)
{
	ProcessFont(L"Segoe UI", 92, fontOutputDirectory);
	ProcessFont(L"Segoe UI Light", 36, fontOutputDirectory);
	ProcessFont(L"Calibri", 16, fontOutputDirectory);
}

int CALLBACK wWinMain(
  _In_  HINSTANCE hInstance,
  _In_  HINSTANCE hPrevInstance,
  _In_  LPWSTR lpCmdLine,
  _In_  int nCmdShow
)
{
	int argc;
	wchar_t** argv = CommandLineToArgvW(lpCmdLine, &argc);

	wcout << endl << "Starting up PostProcessor. Got " << argc << " arguments: " << endl;
	for (int i = 0; i < argc; i++)
	{
		wcout << L"\"" << argv[i] << L"\" ";
	}
	wcout << endl;

	if (argc != 6)
	{
		wchar_t exeName[MAX_PATH];
		GetModuleFileName(nullptr, exeName, MAX_PATH);

		wcout << L"Invalid number of arguments! Usage: " << exeName << L" <shaderDirectory> <modelInputDirectory> <modelOutputDirectory>" 
			<< L" <animatedModelInputDirectory> <animatedModelOutputDirectory> <fontOutputDirectory>" << endl;
		return -1;
	}
	
	wcout << endl;
	ProcessShaders(argv[0]);
	ProcessModels(argv[1], argv[2]);
	ProcessAnimatedModels(argv[3], argv[4]);
	ProcessFonts(argv[5]);
	
	LocalFree(argv);
	return 0;
}