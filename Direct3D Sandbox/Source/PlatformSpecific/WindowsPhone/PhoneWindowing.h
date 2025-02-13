#pragma once
#if WINDOWS_PHONE

#include "Tools.h"

class PhoneWindowing
{
private:
	int m_Width;
	int m_Height;
	HWND m_WindowHandle;

	Windows::System::Display::DisplayRequest^ m_DisplayRequest;

public:
	PhoneWindowing();
	~PhoneWindowing();

	void DispatchMessages() const;

	inline int GetWidth() const { return m_Width; }
	inline int GetHeight() const { return m_Height; }
	inline float GetAspectRatio() const { return static_cast<float>(m_Width) / static_cast<float>(m_Height); }
	inline bool IsFullscreen() const { return false; }
	inline HWND GetWindowHandle() const { return m_WindowHandle; }
};

ref class PhoneFrameworkView sealed : public Windows::ApplicationModel::Core::IFrameworkView
{
public:
	PhoneFrameworkView();
	
	virtual void Initialize(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView);
	virtual void SetWindow(Windows::UI::Core::CoreWindow^ window);
	virtual void Load(Platform::String^ entryPoint);
	virtual void Run();
	virtual void Uninitialize();

protected:
	void OnActivated(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView, Windows::ApplicationModel::Activation::IActivatedEventArgs^ args);
	void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ args);
	void OnResuming(Platform::Object^ sender, Platform::Object^ args);
	void OnBackButtonPressed(Platform::Object^ sender, Windows::Phone::UI::Input::BackPressedEventArgs^ args);
	void OnWindowClosed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::CoreWindowEventArgs^ args);
	void OnVisibilityChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::VisibilityChangedEventArgs^ args);
	void OnPointerPressed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args);
	void OnPointerMoved(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args);
	void OnPointerReleased(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args);
	void OnKeyDown(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args);
	void OnKeyUp(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args);

private:
	int numberOfFingersDown;

	map<int, Point2D> cursorPosition;
};

ref class Direct3DApplicationSource sealed : Windows::ApplicationModel::Core::IFrameworkViewSource
{
public:
	virtual Windows::ApplicationModel::Core::IFrameworkView^ CreateView();
};

typedef PhoneWindowing Windowing;

#endif	// WINDOWS_PHONE