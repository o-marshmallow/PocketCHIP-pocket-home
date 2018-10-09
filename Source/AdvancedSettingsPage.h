#ifndef ADVANCEDSETTINGS_H
#define ADVANCEDSETTINGS_H

#include "../JuceLibraryCode/JuceHeader.h"
#include "SettingsPageLogin.h"
#include "PersonalizePageComponent.h"
#include "InputSettingsPage.h"
#include "Utils.h"
#include "Main.h"
#include "DateTimePage.h"

#define OPTPERPAGE 4
using namespace juce;

class AdvancedSettingsPage : public Component, public Button::Listener{
public:
  AdvancedSettingsPage(LauncherComponent*);
  ~AdvancedSettingsPage();
  void buttonClicked(Button*) override;
  void paint(Graphics&) override;
  void resized() override;
  void deleteIcon(String, String);
  void displayNoPassword();

private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AdvancedSettingsPage)
 
  void checkNav();
  //Title of the pane
  Label title;
  //BackButton
  ScopedPointer<ImageButton> backButton;
  //Background color
  Colour bg_color;
  
  //Pages with associated buttons
  TextButton addLogin;
  TextButton removeLogin;
  TextButton personalizeButton;
  TextButton dateandtime;
  TextButton inputoptions;
  ScopedPointer<SettingsPageLogin> spl;
  ScopedPointer<PersonalizePageComponent> ppc;
  ScopedPointer<DateTimePage> datetime;
  ScopedPointer<InputSettingsPage> inputsettings;
  int optPerPage;

  //Next and previous buttons
  ScopedPointer<ImageButton> previousarrow;
  ScopedPointer<ImageButton> nextarrow;
  
  //Array of buttons
  std::vector<TextButton*> allbuttons;
  int index;
};

#endif
