#pragma once
#include "framework.h"

template <typename T>
T convert_from_abi(com_ptr<::IInspectable> from)
{
	T to{ nullptr }; // `T` is a projected type.

	winrt::check_hresult(from->QueryInterface(winrt::guid_of<T>(),
		winrt::put_abi(to)));

	return to;
}

static DependencyObject FindDescendantByName(DependencyObject root, hstring name)
{
	if (root == nullptr) return nullptr;

	int count = VisualTreeHelper::GetChildrenCount(root);
	for (int i = 0; i < count; i++)
	{
		DependencyObject child = VisualTreeHelper::GetChild(root, i);
		if (child == nullptr) continue;

		hstring childName = child.GetValue(FrameworkElement::NameProperty()).as<hstring>();
		if (childName == name)
			return child;

		DependencyObject result = FindDescendantByName(child, name);
		if (result != nullptr)
			return result;
	}

	return nullptr;
}

static DWORD GetVal(LPCWSTR val)
{
	DWORD dw, dwSize = sizeof(DWORD);
	RegGetValue(HKEY_CURRENT_USER, L"Software\\TranslucentSM", val, RRF_RT_DWORD, NULL, &dw, &dwSize);
	return dw;
}

static HRESULT SetVal(HKEY subkey, LPCWSTR key, DWORD val)
{
	return RegSetValueEx(subkey, key, 0, REG_DWORD, (const BYTE*)&val, sizeof(val));
}