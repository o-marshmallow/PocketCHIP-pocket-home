#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include "Grid.h"

class AppsPageComponent;
class LauncherComponent;

class AppCheckTimer : public Timer {
public:
  AppCheckTimer() {};
  virtual void timerCallback() override;
  // TODO: better pointer usage, weakref for cycle relationship?
  AppsPageComponent* appsPage;
};

class AppDebounceTimer : public Timer {
public:
  AppDebounceTimer() {};
  virtual void timerCallback() override;
  // TODO: better pointer usage, weakref for cycle relationship?
  AppsPageComponent* appsPage;
};

class AppIconButton : public DrawableButton {
public:
  AppIconButton(const String &label, const String &shell, const Drawable *image);
  
  String shell;
  
  Rectangle<float> getImageBounds() const override;
};

enum NavDirection{
  HORIZONTAL, VERTICAL
};

class AppListComponent : 
public Component, public Button::Listener{
public:
  AppListComponent(Component* parent = nullptr, bool = true);
  ~AppListComponent();
  
  ScopedPointer<Grid> grid;
  
  OwnedArray<Component> gridIcons;
  OwnedArray<DrawableImage> iconDrawableImages;
  ScopedPointer<ImageButton> nextPageBtn;
  ScopedPointer<ImageButton> prevPageBtn;
  
  void resized() override;
  void checkShowPageNav();
  
  void next();
  void previous();
  
  void addAndOwnIcon(const String &name, Component *icon);
  DrawableButton *createAndOwnIcon(const String &name, const String &iconPath, const String &shell);
  
  void removeIcon(Component*);
  virtual Array<DrawableButton *> createIconsFromJsonArray(const var &json);
  
  void buttonStateChanged(Button*) override {};
  void buttonClicked(Button *button) override {};
  
  void mouseDrag(const MouseEvent&) override {};
  // FIXME: this is barsize from launcher component
  double btnHeight = 50;
  
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AppListComponent)
  NavDirection direction;
};

enum Choices{
  EDIT      = 1,
  MOVELEFT  = 2,
  MOVERIGHT = 3,
  DELETE    = 4
};

class EditWindow: public Component, public Button::Listener{
public:
  EditWindow(AppIconButton*, int w, int h);
  ~EditWindow();
  virtual void paint(Graphics &) override;
  virtual void resized() override;
  virtual bool invoke();
  virtual void buttonClicked(Button*) override;
  virtual void mouseDown(const MouseEvent&) override;
  virtual String getName();
  virtual String getIcon();
  virtual String getShell();
  
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EditWindow)  
  AppIconButton* button;
  Label lname;
  TextEditor name;
  Label licon;
  TextEditor icon;
  Label lshell;
  TextEditor shell;
  TextButton apply;
  TextButton cancel;
  TextButton browse;
  bool choice;
};

class AppsPageComponent : public AppListComponent{
public:
  AppsPageComponent(LauncherComponent* launcherComponent, bool);
  ~AppsPageComponent();
  
  Array<DrawableButton *> createIconsFromJsonArray(const var &json) override;
  
  OwnedArray<ChildProcess> runningApps;
  
  virtual void buttonStateChanged(Button*) override;
  void buttonClicked(Button *button) override;
  void buttonClicked(const MouseEvent&);
  void mouseDrag(const MouseEvent&) override;
  void mouseUp(const MouseEvent&) override;
  bool keyPressed (const KeyPress &) override;
  
  void checkRunningApps();
  
  bool debounce = false;

private:
  using AppRunningMap = HashMap<AppIconButton*, int>;
  ScopedPointer<Drawable> cpy;

  DrawableButton* appsLibraryBtn;
  LauncherComponent* launcherComponent;
  
  AppRunningMap runningAppsByButton;
  AppCheckTimer runningCheckTimer;
  AppDebounceTimer debounceTimer;

  void onTrash(Button*);
  void startApp(AppIconButton* appButton);
  void focusApp(AppIconButton* appButton, const String& windowId);
  void startOrFocusApp(AppIconButton* appButton);
  void openAppsLibrary();
  void updateIcon(AppIconButton*, EditWindow*);
  void manageChoice(AppIconButton*, int);
  void moveInConfig(AppIconButton*, bool);
  
  //Trash Icon
  ScopedPointer<ImageButton> trashButton;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AppsPageComponent)
  
  //Store mouse dragged event X and Y
  int x;
  int y;
  bool shouldMove;
};

class NavigationListener : public Button::Listener {
public:
    NavigationListener(Button*, AppListComponent*);
    void buttonClicked(Button *button) override;

private:
    Button* next;
    AppListComponent* page;
};
