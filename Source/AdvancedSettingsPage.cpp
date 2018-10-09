#include "AdvancedSettingsPage.h"

AdvancedSettingsPage::AdvancedSettingsPage(LauncherComponent* lc):
bg_color(0xffd23c6d),
title("settings", "Advanced Settings"),
addLogin("Change your password"), 
removeLogin("Remove your password"),
personalizeButton("Personalize your homepage"),
dateandtime("Date and time"),
inputoptions("Input settings"),
spl(new SettingsPageLogin),
datetime(new DateTimePage(lc)),
ppc(new PersonalizePageComponent(lc)),
inputsettings(new InputSettingsPage(lc)),
index(0),
optPerPage(4)
{
  //Title font
  title.setFont(Font(27.f));
  //Back button
  backButton = createImageButton("Back", createImageFromFile(assetFile("backIcon.png")));
  backButton->addListener(this);
  backButton->setAlwaysOnTop(true);
  
  
  addLogin.addListener(this);
  removeLogin.addListener(this);
  personalizeButton.addListener(this);
  dateandtime.addListener(this);
  inputoptions.addListener(this);
  addAndMakeVisible(title);
  addAndMakeVisible(backButton);
  addAndMakeVisible(addLogin);
  addAndMakeVisible(removeLogin);
  addAndMakeVisible(personalizeButton);
  addAndMakeVisible(dateandtime);
  addAndMakeVisible(inputoptions);
  
  //Adding to our buttons
  allbuttons.push_back(&personalizeButton);
  allbuttons.push_back(&addLogin);
  allbuttons.push_back(&removeLogin);
  allbuttons.push_back(&dateandtime);
  allbuttons.push_back(&inputoptions);
  
  //Creating the previous and next arrows images
  previousarrow = createImageButton("Previous", createImageFromFile(assetFile("pageUpIcon.png")));
  nextarrow = createImageButton("Next", createImageFromFile(assetFile("pageDownIcon.png")));
  //Adding the images to the view
  addChildComponent(previousarrow);
  addChildComponent(nextarrow);
  //Adding lsiteners to it
  previousarrow->addListener(this);
  nextarrow->addListener(this);
  
  checkNav();
}

AdvancedSettingsPage::~AdvancedSettingsPage(){ }

void AdvancedSettingsPage::checkNav(){
  if(index > 0){
    title.setVisible(false);
    previousarrow->setVisible(true);
  }
  else{
    title.setVisible(true);
    previousarrow->setVisible(false);
  }
  
  if(index+optPerPage < allbuttons.size())
    nextarrow->setVisible(true);
  else nextarrow->setVisible(false);
}

void AdvancedSettingsPage::resized(){
  auto bounds = getLocalBounds();
  int btn_height = 30;
  
  backButton->setBounds(bounds.getX(), bounds.getY(), 60, bounds.getHeight());
  title.setBounds((bounds.getX()+(bounds.getWidth()/2))-50, bounds.getY()+10, bounds.getWidth(), btn_height);
  
  //Hide all the buttons
  for(int i = 0; i < allbuttons.size(); i++)
    allbuttons[i]->setVisible(false);
 
  optPerPage = (bounds.getHeight() - 100) / 50;
  //Display only the buttons of the current page
  int y = 50;
  for(int i=index; i < allbuttons.size() && i < index+optPerPage; i++){
    allbuttons[i]->setVisible(true);
    allbuttons[i]->setBounds(bounds.getX()+70, bounds.getY()+y, bounds.getWidth() - 80, btn_height);
    y += 50;
  }
  
  int arrow_height = 50;
  nextarrow->setBoundsToFit(0, bounds.getHeight()-arrow_height, bounds.getWidth(), arrow_height, Justification::centred, true);
  previousarrow->setBoundsToFit(0, 0, bounds.getWidth(), arrow_height, Justification::centred, true);
  
  checkNav();
}

void AdvancedSettingsPage::deleteIcon(String name, String shell){
  ppc->deleteIcon(name, shell);
}

void AdvancedSettingsPage::displayNoPassword(){
AlertWindow::showMessageBoxAsync(AlertWindow::AlertIconType::WarningIcon,
                                  "Error", 
                                  "No password is set, cannot remove",
                                  "Ok");
}

void AdvancedSettingsPage::buttonClicked(Button* button){
  if (button == backButton)
    getMainStack().popPage(PageStackComponent::kTransitionTranslateHorizontal);
  else if(button == &addLogin){
    spl->switchToModify();
    getMainStack().pushPage(spl, PageStackComponent::kTransitionTranslateHorizontal);
  }
  else if(button == &removeLogin){
    if(spl->hasPassword()){
      spl->switchToRemove();
      getMainStack().pushPage(spl, PageStackComponent::kTransitionTranslateHorizontal);
    }
    else displayNoPassword();
  }
  else if(button == &personalizeButton)
    getMainStack().pushPage(ppc, PageStackComponent::kTransitionTranslateHorizontal);
  else if(button == &(*nextarrow)){
    index += optPerPage;
    resized();
  }
  else if(button == &(*previousarrow)){
    index -= optPerPage;
    resized();
  }
  else if(button == &inputoptions){
    getMainStack().pushPage(inputsettings, PageStackComponent::kTransitionTranslateHorizontal);
  }
  else if(button == &dateandtime){
    getMainStack().pushPage(datetime, PageStackComponent::kTransitionTranslateHorizontal);
  }
}

void AdvancedSettingsPage::paint(Graphics& g){
    auto bounds = getLocalBounds();
    g.fillAll(bg_color);
}
