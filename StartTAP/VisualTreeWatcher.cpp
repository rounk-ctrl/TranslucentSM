#include "VisualTreeWatcher.h"
using namespace winrt;
using namespace Windows::Foundation;
using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::UI;
using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Xaml::Controls;
using namespace winrt::Windows::UI::Xaml::Media;

#pragma region Helpers
template <typename T>
T convert_from_abi(com_ptr<::IInspectable> from)
{
	T to{ nullptr }; // `T` is a projected type.

	winrt::check_hresult(from->QueryInterface(winrt::guid_of<T>(),
		winrt::put_abi(to)));

	return to;
}

DependencyObject FindDescendantByName(DependencyObject root, hstring name)
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

		DependencyObject result = FindDescendantByName(child, name);
		if (result != nullptr)
		{
			return result;
		}
	}

	return nullptr;
}
#pragma endregion

HRESULT AddSettingsPanel(Grid rootGrid);
DWORD dwRes = 0, dwSize = sizeof(DWORD), dwOpacity = 0, dwLuminosity = 0, dwHide = 0, dwBorder = 0, dwRec = 0;

int64_t token = NULL;

VisualTreeWatcher::VisualTreeWatcher(winrt::com_ptr<IUnknown> site) :
	m_XamlDiagnostics(site.as<IXamlDiagnostics>())
{
	winrt::check_hresult(m_XamlDiagnostics.as<IVisualTreeService3>()->AdviseVisualTreeChange(this));
}
HRESULT VisualTreeWatcher::OnElementStateChanged(InstanceHandle, VisualElementState, LPCWSTR) noexcept
{
	return S_OK;
}

HRESULT VisualTreeWatcher::OnVisualTreeChange(ParentChildRelation relation, VisualElement element, VisualMutationType mutationType)
{
	if (mutationType == Add)
	{
		const std::wstring_view type{ element.Type, SysStringLen(element.Type) };
		const std::wstring_view name{ element.Name, SysStringLen(element.Name) };
		if (name == L"AcrylicBorder")
		{
			RegGetValue(HKEY_CURRENT_USER, L"Software\\TranslucentSM", L"TintOpacity", RRF_RT_DWORD, NULL, &dwOpacity, &dwSize);
			RegGetValue(HKEY_CURRENT_USER, L"Software\\TranslucentSM", L"TintLuminosityOpacity", RRF_RT_DWORD, NULL, &dwLuminosity, &dwSize);

			// apply the things
			if (dwOpacity > 100) dwOpacity = 100;
			if (dwLuminosity > 100) dwLuminosity = 100;
			Border acrylicBorder = FromHandle<Border>(element.Handle);
			acrylicBorder.Background().as<AcrylicBrush>().TintOpacity(double(dwOpacity) / 100);
			acrylicBorder.Background().as<AcrylicBrush>().TintLuminosityOpacity(double(dwLuminosity) / 100);

		}
		else if (name == L"BackgroundElement" && type == L"Windows.UI.Xaml.Controls.Border")
		{
			// w10 hamburger menu fix
			auto backElement = FromHandle<Border>(element.Handle);
			backElement.Background().as<AcrylicBrush>().TintOpacity(0);
		}
		else if (type == L"StartDocked.SearchBoxToggleButton")
		{
			RegGetValue(HKEY_CURRENT_USER, L"Software\\TranslucentSM", L"HideSearch", RRF_RT_DWORD, NULL, &dwHide, &dwSize);
			Control srch = FromHandle<Control>(element.Handle);
			if (dwHide == 1) srch.Visibility(Visibility::Collapsed);

		}
		else if (name == L"RootGrid")
		{
			Grid rootContent = FromHandle<Grid>(element.Handle);
			AddSettingsPanel(rootContent);
		}
		else if (name == L"AcrylicOverlay")
		{
			RegGetValue(HKEY_CURRENT_USER, L"Software\\TranslucentSM", L"HideBorder", RRF_RT_DWORD, NULL, &dwBorder, &dwSize);
			auto acrylicOverlay = FromHandle<Border>(element.Handle);
			if (dwBorder == 1) acrylicOverlay.Background().as<SolidColorBrush>().Opacity(0);
		}
		else if (name == L"TopLevelSuggestionsListHeader" || name == L"SuggestionsParentContainer" || name == L"ShowMoreSuggestions")
		{
			RegGetValue(HKEY_CURRENT_USER, L"Software\\TranslucentSM", L"HideRecommended", RRF_RT_DWORD, NULL, &dwRec, &dwSize);
			auto elmnt = FromHandle<FrameworkElement>(element.Handle);
			if (dwRec == 1) elmnt.Visibility(Visibility::Collapsed);
		}
		else if (name == L"StartMenuPinnedList")
		{
			RegGetValue(HKEY_CURRENT_USER, L"Software\\TranslucentSM", L"HideRecommended", RRF_RT_DWORD, NULL, &dwRec, &dwSize);
			if (dwRec == 0) return S_OK;


			auto topLevelRoot = FromHandle<FrameworkElement>(relation.Parent);
			static auto suggHeader = FindDescendantByName(topLevelRoot, L"TopLevelSuggestionsListHeader").as<FrameworkElement>();
			static auto suggContainer = FindDescendantByName(topLevelRoot, L"SuggestionsParentContainer").as<FrameworkElement>();
			static auto suggBtn = FindDescendantByName(topLevelRoot, L"ShowMoreSuggestions").as<FrameworkElement>();

			auto pinList = FromHandle<FrameworkElement>(element.Handle);
			static auto x = pinList.Height();
			DependencyProperty heightProp = FrameworkElement::HeightProperty();
			if (token == NULL)
			{
				token = pinList.RegisterPropertyChangedCallback(heightProp,
					[](DependencyObject sender, DependencyProperty property)
					{
						auto element = sender.try_as<FrameworkElement>();
						element.Height(x + suggHeader.ActualHeight() + suggContainer.ActualHeight() + suggBtn.ActualHeight());

					});
			}
			pinList.Height(pinList.Height() + suggHeader.ActualHeight() + suggContainer.ActualHeight() + suggBtn.ActualHeight());
		}
		//ChangeLayout(name, type, element);
	}
	return S_OK;
}

// skull
HRESULT VisualTreeWatcher::ChangeLayout(std::wstring_view name, std::wstring_view type, VisualElement element)
{
	if (name == L"AllAppsRoot")
	{
		Grid allAppsRoot = FromHandle<Grid>(element.Handle);
		Windows::UI::Xaml::Media::Media3D::CompositeTransform3D k;
		k.TranslateX(-950);
		allAppsRoot.Transform3D(k);
		allAppsRoot.Width(350);
		DependencyProperty visibilityProperty = UIElement::VisibilityProperty();
		allAppsRoot.RegisterPropertyChangedCallback(visibilityProperty,
			[](DependencyObject sender, DependencyProperty property)
			{
				auto element = sender.try_as<FrameworkElement>();
				element.Visibility(Visibility::Visible);
			});
		allAppsRoot.Visibility(Visibility::Visible);

		auto allAppsPanel = FindDescendantByName(allAppsRoot, L"AllAppsPanel");
		auto sZoom = VisualTreeHelper::GetChild(allAppsPanel, 0).as<SemanticZoom>();
		sZoom.Margin({ -32,0,0,0 });
		}
	else if (name == L"AllAppsPaneHeader")
	{
		Grid allAppsRoot = FromHandle<Grid>(element.Handle);
		allAppsRoot.Margin({ 83,0,64,0 });
		}
	else if (name == L"AppsList")
	{
		auto appList = FromHandle<Control>(element.Handle);
		appList.Padding({ 52,3,-32,32 });
		}
	else if (type == L"StartDocked.StartSizingFrame")
	{
		auto startFrame = FromHandle<Control>(element.Handle);
		startFrame.MaxWidth(900);
		startFrame.Width(900);
		}
	else if (name == L"TopLevelRoot")
	{
		auto topRoot = FromHandle<Grid>(element.Handle);
		topRoot.Width(630);
		topRoot.HorizontalAlignment(HorizontalAlignment::Right);
		}
	else if (name == L"ShowAllAppsButton" || name == L"CloseAllAppsButton")
	{
		auto btn = FromHandle<FrameworkElement>(element.Handle);
		btn.Visibility(Visibility::Collapsed);
		}
}

HRESULT AddSettingsPanel(Grid rootGrid)
{
	RegGetValue(HKEY_CURRENT_USER, L"Software\\TranslucentSM", L"TintOpacity", RRF_RT_DWORD, NULL, &dwOpacity, &dwSize);
	RegGetValue(HKEY_CURRENT_USER, L"Software\\TranslucentSM", L"TintLuminosityOpacity", RRF_RT_DWORD, NULL, &dwLuminosity, &dwSize);
	RegGetValue(HKEY_CURRENT_USER, L"Software\\TranslucentSM", L"HideSearch", RRF_RT_DWORD, NULL, &dwHide, &dwSize);
	RegGetValue(HKEY_CURRENT_USER, L"Software\\TranslucentSM", L"HideBorder", RRF_RT_DWORD, NULL, &dwBorder, &dwSize);
	RegGetValue(HKEY_CURRENT_USER, L"Software\\TranslucentSM", L"HideRecommended", RRF_RT_DWORD, NULL, &dwRec, &dwSize);


	static Border acrylicBorder = FindDescendantByName(rootGrid, L"AcrylicBorder").as<Border>();

	Button bt;
	auto f = FontIcon();
	f.FontSize(16);
	bt.BorderThickness(Thickness{ 0 });
	bt.Background(SolidColorBrush(Colors::Transparent()));
	auto stackPanel = StackPanel();

	TextBlock tbx;
	tbx.FontFamily(Windows::UI::Xaml::Media::FontFamily(L"Segoe UI Variable"));
	tbx.FontSize(13.0);
	winrt::hstring hs = L"TintOpacity";
	tbx.Text(hs);
	stackPanel.Children().Append(tbx);

	Slider slider;
	slider.Width(140);
	slider.Value(dwOpacity);
	stackPanel.Children().Append(slider);

	TextBlock tbx1;
	tbx1.FontFamily(Windows::UI::Xaml::Media::FontFamily(L"Segoe UI Variable"));
	tbx1.FontSize(13.0);
	winrt::hstring hs1 = L"TintLuminosityOpacity";
	tbx1.Text(hs1);
	stackPanel.Children().Append(tbx1);

	Slider slider2;
	slider2.Width(140);
	slider2.Value(dwLuminosity);
	stackPanel.Children().Append(slider2);

	static HKEY subKey = nullptr;
	DWORD dwSize = sizeof(DWORD), dwInstalled = 0;
	RegCreateKeyEx(HKEY_CURRENT_USER, L"SOFTWARE\\TranslucentSM", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &subKey, NULL);

	slider.ValueChanged([](Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&) {
		auto sliderControl = sender.as<Slider>();
		double sliderValue = sliderControl.Value();
		DWORD val = sliderValue;
		acrylicBorder.Background().as<AcrylicBrush>().TintOpacity(double(sliderValue) / 100);
		RegSetValueEx(subKey, TEXT("TintOpacity"), 0, REG_DWORD, (const BYTE*)&val, sizeof(val));
		});
	slider2.ValueChanged([](Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&) {
		auto sliderControl = sender.as<Slider>();
		double sliderValue = sliderControl.Value();
		DWORD val = sliderValue;
		acrylicBorder.Background().as<AcrylicBrush>().TintLuminosityOpacity(double(sliderValue) / 100);
		RegSetValueEx(subKey, TEXT("TintLuminosityOpacity"), 0, REG_DWORD, (const BYTE*)&val, sizeof(val));
		});

	
	static auto srchBox = FindDescendantByName(rootGrid, L"StartMenuSearchBox").as<Control>();
	if (srchBox != nullptr)
	{
		auto checkBox = CheckBox();
		checkBox.Content(box_value(L"Hide search box"));
		stackPanel.Children().Append(checkBox);
		if (dwHide == 1)
		{
			checkBox.IsChecked(true);
		}
		checkBox.Checked([](Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&) {
			DWORD ser = 1;
			RegSetValueEx(subKey, TEXT("HideSearch"), 0, REG_DWORD, (const BYTE*)&ser, sizeof(ser));
			srchBox.Visibility(Visibility::Collapsed);
			});

		checkBox.Unchecked([](Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&) {
			DWORD ser = 0;
			RegSetValueEx(subKey, TEXT("HideSearch"), 0, REG_DWORD, (const BYTE*)&ser, sizeof(ser));
			srchBox.Visibility(Visibility::Visible);
			});
	}

	
	static auto acrylicOverlay = FindDescendantByName(rootGrid, L"AcrylicOverlay").as<Border>();
	if (acrylicOverlay != nullptr)
	{
		auto checkBox = CheckBox();
		checkBox.Content(box_value(L"Hide white border"));
		stackPanel.Children().Append(checkBox);
		if (dwBorder == 1)
		{
			checkBox.IsChecked(true);
		}
		checkBox.Checked([](Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&) {
			DWORD ser = 1;
			RegSetValueEx(subKey, TEXT("HideBorder"), 0, REG_DWORD, (const BYTE*)&ser, sizeof(ser));
			acrylicOverlay.Background().as<SolidColorBrush>().Opacity(0);
			});

		checkBox.Unchecked([](Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&) {
			DWORD ser = 0;
			RegSetValueEx(subKey, TEXT("HideBorder"), 0, REG_DWORD, (const BYTE*)&ser, sizeof(ser));
			acrylicOverlay.Background().as<SolidColorBrush>().Opacity(1);
			});
	}

	static auto topRoot = FindDescendantByName(rootGrid, L"TopLevelRoot").as<Grid>();
	if (topRoot != nullptr)
	{
		auto checkBox = CheckBox();
		checkBox.Content(box_value(L"Hide recommended"));
		stackPanel.Children().Append(checkBox);
		if (dwRec == 1)
		{
			checkBox.IsChecked(true);
		}

		static auto suggHeader = FindDescendantByName(topRoot, L"TopLevelSuggestionsListHeader").as<FrameworkElement>();
		static auto suggContainer = FindDescendantByName(topRoot, L"SuggestionsParentContainer").as<FrameworkElement>();
		static auto suggBtn = FindDescendantByName(topRoot, L"ShowMoreSuggestions").as<FrameworkElement>();
		
		static auto pinList = FindDescendantByName(topRoot, L"StartMenuPinnedList").as<FrameworkElement>();
		static auto x = pinList.Height();

		checkBox.Checked([](Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&)
			{
				DWORD ser = 1;
				RegSetValueEx(subKey, TEXT("HideRecommended"), 0, REG_DWORD, (const BYTE*)&ser, sizeof(ser));
				DependencyProperty heightProp = FrameworkElement::HeightProperty();
				if (token == NULL)
				{
					token = pinList.RegisterPropertyChangedCallback(heightProp,
						[](DependencyObject sender, DependencyProperty property)
						{
							auto element = sender.try_as<FrameworkElement>();
							element.Height(x + suggHeader.ActualHeight() + suggContainer.ActualHeight() + suggBtn.ActualHeight());
						});
				}
				pinList.Height(pinList.Height() + suggHeader.ActualHeight() + suggContainer.ActualHeight() + suggBtn.ActualHeight());
				suggHeader.Visibility(Visibility::Collapsed);
				suggContainer.Visibility(Visibility::Collapsed);
				suggBtn.Visibility(Visibility::Collapsed);
			});

		checkBox.Unchecked([](Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&)
			{
				DWORD ser = 0;
				RegSetValueEx(subKey, TEXT("HideRecommended"), 0, REG_DWORD, (const BYTE*)&ser, sizeof(ser));
				suggHeader.Visibility(Visibility::Visible);
				suggContainer.Visibility(Visibility::Visible);
				suggBtn.Visibility(Visibility::Visible);

				DependencyProperty heightProp = FrameworkElement::HeightProperty();
				if (token != NULL)
				{
					pinList.UnregisterPropertyChangedCallback(heightProp, token);
					token = NULL;
				}
				pinList.Height(pinList.Height() - suggHeader.ActualHeight() - suggContainer.ActualHeight() - suggBtn.ActualHeight());
			});
	}


	auto nvds = FindDescendantByName(rootGrid, L"RootPanel");
	auto nvpane = FindDescendantByName(rootGrid, L"NavigationPanePlacesListView");
	auto rootpanel = FindDescendantByName(nvpane, L"Root");
	// windows 11
	if (rootpanel != nullptr)
	{
		auto grid = VisualTreeHelper::GetChild(rootpanel, 0).as<Grid>();
		if (grid != nullptr)
		{
			ToolTipService::SetToolTip(bt, box_value(L"TranslucentSM settings"));

			f.Glyph(L"\uE104");
			f.FontFamily(Media::FontFamily(L"Segoe Fluent Icons"));

			bt.Content(winrt::box_value(f));
			bt.Margin({ -40,0,0,0 });
			bt.Padding({ 11.2,11.2,11.2,11.2 });
			bt.Width(38);
			bt.BorderBrush(SolidColorBrush(Colors::Transparent()));
			grid.Children().Append(bt);


			Flyout flyout;
			flyout.Content(stackPanel);
			bt.Flyout(flyout);
		}
	}
	// windows 10
	else if (nvds != nullptr)
	{
		auto nvGrid = nvds.as<Grid>();

		// w10 start menu
		f.Glyph(L"\uE70F");
		f.FontFamily(Media::FontFamily(L"Segoe MDL2 Assets"));

		RevealBorderBrush rv;
		rv.TargetTheme(ApplicationTheme::Dark);

		RevealBorderBrush rvb;
		rvb.TargetTheme(ApplicationTheme::Dark);
		rvb.Color(ColorHelper::FromArgb(128, 255, 255, 255));
		rvb.Opacity(0.4);

		// doesnt seem to work on windows 11
		bt.Resources().Insert(winrt::box_value(L"ButtonBackgroundPointerOver"), rvb);
		bt.Resources().Insert(winrt::box_value(L"ButtonBackgroundPressed"), rvb);

		f.HorizontalAlignment(HorizontalAlignment::Left);
		f.Margin({ 11.2,11.2,11.2,11.2 });

		auto mainpanel = StackPanel();
		mainpanel.Margin({ 0,-94,0,0 });
		mainpanel.Height(45);
		mainpanel.BorderThickness({ 1,1,1,1 });
		mainpanel.BorderBrush(rv);
		Grid::SetRow(mainpanel, 3);
		Canvas::SetZIndex(mainpanel, 10);
		mainpanel.Children().Append(bt);

		auto textBlock = TextBlock();
		textBlock.Text(L"TranslucentSM settings");
		textBlock.Margin({ 5,13,0,0 });
		textBlock.HorizontalAlignment(HorizontalAlignment::Left);
		textBlock.FontSize(15);

		auto panel = StackPanel();
		panel.Margin({ -4,-4,0,0 });
		panel.Children().Append(f);
		panel.Children().Append(textBlock);
		panel.Orientation(Orientation::Horizontal);
		panel.Height(45);
		panel.Width(256);
		panel.HorizontalAlignment(HorizontalAlignment::Left);

		bt.Height(45);
		bt.Width(256);
		bt.Content(panel);

		slider.Width(230);
		slider2.Width(230);

		// flyout
		Flyout flyout;
		flyout.Content(stackPanel);
		bt.Flyout(flyout);

		nvGrid.Children().InsertAt(0, mainpanel);
	}
}