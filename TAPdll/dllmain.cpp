// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

#include <winrt/base.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/Windows.UI.Xaml.Controls.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>

using namespace winrt;
using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::UI;
using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Xaml::Controls;
using namespace winrt::Windows::UI::Xaml::Media;

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
template <typename T>
T convert_from_abi(com_ptr<::IInspectable> from)
{
	T to{ nullptr }; // `T` is a projected type.

	winrt::check_hresult(from->QueryInterface(winrt::guid_of<T>(),
		winrt::put_abi(to)));

	return to;
}

DependencyObject FindDescendentByName(DependencyObject root, hstring name)
{
	if (root == nullptr)
	{
		return nullptr;
	}

	int count = VisualTreeHelper::GetChildrenCount(root);
	for (int i = 0; i < count; i++)
	{
		DependencyObject child = VisualTreeHelper::GetChild(root, i);
		if (child == nullptr)
		{
			continue;
		}

		hstring childName = child.GetValue(FrameworkElement::NameProperty()).as<hstring>();
		if (childName == name)
		{
			return child;
		}

		DependencyObject result = FindDescendentByName(child, name);
		if (result != nullptr)
		{
			return result;
		}
	}

	return nullptr;
}

DWORD dwRes = 0, dwSize = sizeof(DWORD), dwOpacity = 0;
struct ExplorerTAP : winrt::implements<ExplorerTAP, IObjectWithSite>
{	
	HRESULT STDMETHODCALLTYPE SetSite(IUnknown* pUnkSite) noexcept override
	{
		site.copy_from(pUnkSite);
		com_ptr<IXamlDiagnostics> diag = site.as<IXamlDiagnostics>();
		com_ptr<::IInspectable> dispatcherPtr;
		diag->GetDispatcher(dispatcherPtr.put());
		CoreDispatcher dispatcher = convert_from_abi<CoreDispatcher>(dispatcherPtr);
		RegGetValue(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"Start_ShowClassicMode", RRF_RT_DWORD, NULL, &dwRes, &dwSize);
		RegGetValue(HKEY_CURRENT_USER, L"Software\\TranslucentSM", L"TintOpacity", RRF_RT_DWORD, NULL, &dwOpacity, &dwSize);
		
		dispatcher.RunAsync(CoreDispatcherPriority::Normal, []()
			{
				auto content = Window::Current().Content();
				// if (dwRes == 1)
				// {
					/*
					auto canvas = content.as<Canvas>();
					auto startSizingFrame = canvas.Children().GetAt(0).as<ContentControl>();
					auto startSizingFramePanel = startSizingFrame.Content().as<ContentControl>();
					auto contentPresenterFrame = startSizingFramePanel.Content().as<UserControl>();
					auto rootGrid = contentPresenterFrame.Content().as<Grid>();
					auto acrylicBorder = rootGrid.Children().GetAt(3).as<Border>();
					auto acrylicBorderBg = acrylicBorder.Background().as<AcrylicBrush>();
					/// acrylic
					if (dwOpacity > 10) dwOpacity = 10;
					acrylicBorderBg.TintOpacity(double(dwOpacity)/10);
					/*
					/// mica
					/// shit implementation
					acrylicBorder.Background(SolidColorBrush(Colors::Transparent()));
					winrt::Microsoft::UI::Xaml::Controls::BackdropMaterial::SetApplyToRootOrPageBackground(contentPresenterFrame, true);
					*/
					
					// Search for AcrylicBorder name
					auto acrylicBorder = FindDescedentByName(content, L"AcrylicBorder").as<Border>();
					if (acrylicBorder != nullptr)
						acrylicBorder.Background(SolidColorBrush(Colors::Transparent()));
				// }
			});
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE GetSite(REFIID riid, void** ppvSite) noexcept override
	{
		return site.as(riid, ppvSite);
	}

private:
	winrt::com_ptr<IUnknown> site;
};
struct TAPFactory : winrt::implements<TAPFactory, IClassFactory>
{
	HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppvObject) override try
	{
		*ppvObject = nullptr;

		if (pUnkOuter)
		{
			return CLASS_E_NOAGGREGATION;
		}

		return winrt::make<ExplorerTAP>().as(riid, ppvObject);
	}
	catch (...)
	{
		return winrt::to_hresult();
	}

	HRESULT STDMETHODCALLTYPE LockServer(BOOL) noexcept override
	{
		return S_OK;
	}
};
_Use_decl_annotations_ STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv) try
{
	*ppv = nullptr;

	// TODO: move this somewhere common
	// {36162BD3-3531-4131-9B8B-7FB1A991EF51}
	static constexpr GUID temp =
	{ 0x36162bd3, 0x3531, 0x4131, { 0x9b, 0x8b, 0x7f, 0xb1, 0xa9, 0x91, 0xef, 0x51 } };
	if (rclsid == temp)
	{
		return winrt::make<TAPFactory>().as(riid, ppv);
	}
	else
	{
		return CLASS_E_CLASSNOTAVAILABLE;
	}
}
catch (...)
{
	return winrt::to_hresult();
}
_Use_decl_annotations_ STDAPI DllCanUnloadNow(void)
{
	if (winrt::get_module_lock())
	{
		return S_FALSE;
	}
	else
	{

		return S_OK;
	}
}
