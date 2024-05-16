#include "VisualTreeWatcher.h"
#include "Helpers.h"


HRESULT AddSettingsPanel(Grid rootGrid);
DWORD dwSize = sizeof(DWORD), dwOpacity = 0, dwLuminosity = 0, dwHide = 0, dwBorder = 0, dwRec = 0;

int64_t token = NULL;
int64_t token_vis = NULL;
static double pad = 15;
static Thickness oldSrchMar;
static double oldSrchHeight;

static bool rechide = false;
static bool srchhide = false;

VisualTreeWatcher::VisualTreeWatcher(winrt::com_ptr<IUnknown> site)
	: m_selfPtr(this, winrt::take_ownership_from_abi_t{}),
	m_XamlDiagnostics(site.as<IXamlDiagnostics>())
{
	this->AddRef();

	HANDLE thread = CreateThread(
		nullptr, 0,
		[](LPVOID lpParam) -> DWORD {
			auto watcher = reinterpret_cast<VisualTreeWatcher*>(lpParam);
			watcher->AdviseVisualTreeChange();
			return 0;
		},
		this, 0, nullptr);
	if (thread) {
		CloseHandle(thread);
	}
}

void VisualTreeWatcher::AdviseVisualTreeChange() {
	const auto treeService = m_XamlDiagnostics.as<IVisualTreeService3>();
	winrt::check_hresult(treeService->AdviseVisualTreeChange(this));
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
			dwOpacity = GetVal(L"TintOpacity");
			dwLuminosity = GetVal(L"TintLuminosityOpacity");

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
			dwHide = GetVal(L"HideSearch");
			Control srch = FromHandle<Control>(element.Handle);
			oldSrchMar = srch.Margin();
			oldSrchHeight = srch.Height();
			if (dwHide == 1)
			{
				srch.Height(0);
				srch.Margin({0});
			}

			// recommended fix
			if (dwHide == 1) pad = srch.ActualHeight() + srch.Padding().Bottom + srch.Padding().Top + 55;

		}
		else if (name == L"RootGrid")
		{
			Grid rootContent = FromHandle<Grid>(element.Handle);
			if (GetVal(L"EditButton")==1)
			{
				AddSettingsPanel(rootContent);
			}
		}
		else if (name == L"AcrylicOverlay")
		{
			dwBorder = GetVal(L"HideBorder");
			auto acrylicOverlay = FromHandle<Border>(element.Handle);
			if (dwBorder == 1) acrylicOverlay.Background().as<SolidColorBrush>().Opacity(0);
		}
		else if (name == L"SuggestionsParentContainer" || name == L"ShowMoreSuggestions")
		{
			dwRec = GetVal(L"HideRecommended");
			auto elmnt = FromHandle<FrameworkElement>(element.Handle);
			if (dwRec == 1) elmnt.Visibility(Visibility::Collapsed);
		}
		else if (name == L"TopLevelSuggestionsListHeader")
		{
			dwRec = GetVal(L"HideRecommended");
			auto elmnt = FromHandle<FrameworkElement>(element.Handle);
			if (dwRec == 1) elmnt.Visibility(Visibility::Collapsed);
			if (token_vis == NULL)
			{
				token_vis = elmnt.RegisterPropertyChangedCallback(UIElement::VisibilityProperty(),
					[](DependencyObject sender, DependencyProperty property)
					{
						auto element = sender.try_as<FrameworkElement>();
						element.Visibility(rechide ? Visibility::Collapsed : Visibility::Visible);
					});
			}
		}
		else if (name == L"StartMenuPinnedList")
		{
			dwRec = GetVal(L"HideRecommended");
			if (dwRec == 1)
			{
				auto topLevelRoot = FromHandle<FrameworkElement>(relation.Parent);
				static auto suggHeader = FindDescendantByName(topLevelRoot, L"TopLevelSuggestionsListHeader").as<FrameworkElement>();
				static auto suggContainer = FindDescendantByName(topLevelRoot, L"SuggestionsParentContainer").as<FrameworkElement>();
				static auto suggBtn = FindDescendantByName(topLevelRoot, L"ShowMoreSuggestions").as<FrameworkElement>();
				auto pinList = FromHandle<FrameworkElement>(element.Handle);

				static double height = pinList.Height() + suggContainer.ActualHeight() + suggBtn.ActualHeight();
				if (token == NULL)
				{
					token = pinList.RegisterPropertyChangedCallback(FrameworkElement::HeightProperty(),
						[](DependencyObject sender, DependencyProperty property)
						{
							auto element = sender.try_as<FrameworkElement>();
							element.Height(height + pad);
						});
				}

				pinList.Height(height + pad);
				suggHeader.Visibility(Visibility::Collapsed);
				suggContainer.Visibility(Visibility::Collapsed);
				suggBtn.Visibility(Visibility::Collapsed);
			}
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
	dwOpacity = GetVal(L"TintOpacity");
	dwLuminosity = GetVal(L"TintLuminosityOpacity");
	dwHide = GetVal(L"HideSearch");
	dwBorder = GetVal(L"HideBorder");
	dwRec = GetVal(L"HideRecommended");

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
		double sliderValue = sender.as<Slider>().Value();
		acrylicBorder.Background().as<AcrylicBrush>().TintOpacity(double(sliderValue) / 100);

		SetVal(subKey, L"TintOpacity", sliderValue);
		});

	slider2.ValueChanged([](Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&) {
		double sliderValue = sender.as<Slider>().Value();
		acrylicBorder.Background().as<AcrylicBrush>().TintLuminosityOpacity(double(sliderValue) / 100);

		SetVal(subKey, L"TintLuminosityOpacity", sliderValue);
		});


	if (dwRec == 1) rechide = true;
	if (dwHide == 1) srchhide = true;


	auto srchBoxElm = FindDescendantByName(rootGrid, L"StartMenuSearchBox");
	if (srchBoxElm != nullptr)
	{
		static auto srchBox = srchBoxElm.as<Control>();

		auto root = VisualTreeHelper::GetParent(srchBox).as<FrameworkElement>();

		auto checkBox = CheckBox();
		checkBox.Content(box_value(L"Hide search box"));
		stackPanel.Children().Append(checkBox);
		if (dwHide == 1)
		{
			checkBox.IsChecked(true);
			srchhide = true;
			pad = srchBox.ActualHeight() + srchBox.Padding().Bottom + srchBox.Padding().Top + 55;
		}
		// events
		checkBox.Checked([](Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&) {
			SetVal(subKey, L"HideSearch", 1);
			srchhide = true;

			srchBox.Height(0);
			srchBox.Margin({ 0 });
			pad = srchBox.ActualHeight() + srchBox.Padding().Bottom + srchBox.Padding().Top + 55;
			});

		checkBox.Unchecked([](Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&) {
			SetVal(subKey, L"HideSearch", 0);
			srchhide = false;

			srchBox.Height(oldSrchHeight);
			srchBox.Margin(oldSrchMar);
			pad = 15;
			});
	}

	auto acrylicOverlayElm = FindDescendantByName(rootGrid, L"AcrylicOverlay");
	if (acrylicOverlayElm != nullptr)
	{
		static auto acrylicOverlay = acrylicOverlayElm.as<Border>();

		auto checkBox = CheckBox();
		checkBox.Content(box_value(L"Hide white border"));
		stackPanel.Children().Append(checkBox);
		if (dwBorder == 1)
			checkBox.IsChecked(true);

		checkBox.Checked([](Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&) {
			SetVal(subKey, L"HideBorder", 1);
			acrylicOverlay.Background().as<SolidColorBrush>().Opacity(0);
			});

		checkBox.Unchecked([](Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&) {
			SetVal(subKey, L"HideBorder", 0);
			acrylicOverlay.Background().as<SolidColorBrush>().Opacity(1);
			});
	}

	auto topRootElm = FindDescendantByName(rootGrid, L"TopLevelRoot");
	if (topRootElm != nullptr)
	{
		auto topRoot = topRootElm.as<Grid>();

		auto checkBox = CheckBox();
		checkBox.Content(box_value(L"Hide recommended"));
		stackPanel.Children().Append(checkBox);
		if (dwRec == 1)
		{
			checkBox.IsChecked(true);
		}

		static auto suggContainer = FindDescendantByName(topRoot, L"SuggestionsParentContainer").as<FrameworkElement>();
		static auto suggBtn = FindDescendantByName(topRoot, L"ShowMoreSuggestions").as<FrameworkElement>();
		static auto suggHeader = FindDescendantByName(topRoot, L"TopLevelSuggestionsListHeader").as<FrameworkElement>();

		static auto pinList = FindDescendantByName(topRoot, L"StartMenuPinnedList").as<FrameworkElement>();

		static auto pinH = pinList.Height();
		static auto x = pinH + suggContainer.ActualHeight() + suggBtn.ActualHeight();


		checkBox.Checked([](Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&)
			{
				SetVal(subKey, L"HideRecommended", 1);
				rechide = true;

				DependencyProperty heightProp = FrameworkElement::HeightProperty();
				if (token == NULL)
				{
					token = pinList.RegisterPropertyChangedCallback(heightProp,
						[](DependencyObject sender, DependencyProperty property)
						{
							auto element = sender.try_as<FrameworkElement>();
							element.Height(x + pad);
						});
				}
				pinList.Height(x + pad);
				suggHeader.Visibility(Visibility::Collapsed);
				suggContainer.Visibility(Visibility::Collapsed);
				suggBtn.Visibility(Visibility::Collapsed);
			});

		checkBox.Unchecked([](Windows::Foundation::IInspectable const& sender, RoutedEventArgs const&)
			{
				SetVal(subKey, L"HideRecommended", 0);
				rechide = false;

				suggHeader.Visibility(Visibility::Visible);
				suggContainer.Visibility(Visibility::Visible);
				suggBtn.Visibility(Visibility::Visible);

				DependencyProperty heightProp = FrameworkElement::HeightProperty();
				if (token != NULL)
				{
					pinList.UnregisterPropertyChangedCallback(heightProp, token);
					token = NULL;
				}

				int height = pinH;
				pinList.Height(height);

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
