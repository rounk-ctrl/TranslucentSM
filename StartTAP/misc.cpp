#include "misc.h"

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
	return ERROR_SUCCESS;
}
