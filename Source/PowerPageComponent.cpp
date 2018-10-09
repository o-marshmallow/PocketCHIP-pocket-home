#include "PowerPageComponent.h"
#include "PowerPageFelComponent.h"
#include "Main.h"
#include "Utils.h"
#include "PokeLookAndFeel.h"

#include <numeric>

unsigned char PowerPageComponent::rev_number = 9;
unsigned char PowerPageComponent::bug_number = 0;

PowerPageComponent::PowerPageComponent() {
  bgColor = Colours::black;
  bgImage = createImageFromFile(assetFile("powerMenuBackground.png"));
  mainPage = new Component();
  addAndMakeVisible(mainPage);
  mainPage->toBack();
  ChildProcess child{};
  
  felPage = new PowerFelPageComponent();
  
  //Setting up the lockscreen
  auto lambda = [this](){ this->hideLockscreen(); };
  lockscreen = new LoginPage(lambda);
  
  // create back button
  backButton = createImageButton(
      "Back", createImageFromFile(assetFile("nextIcon.png")));
  backButton->addListener(this);
  backButton->setAlwaysOnTop(true);
  addAndMakeVisible(backButton);
    
  powerOffButton = new TextButton("Power OFF");
  powerOffButton->setButtonText("Shutdown");
  powerOffButton->addListener(this);
  addAndMakeVisible(powerOffButton);
    
  rebootButton = new TextButton("Reboot");
  rebootButton->setButtonText("Reboot");
  rebootButton->addListener(this);
  addAndMakeVisible(rebootButton);
    
   sleepButton = new TextButton("Sleep");
   sleepButton->setButtonText("Sleep");
   sleepButton->addListener(this);
   addAndMakeVisible(sleepButton);
    
    felButton = new TextButton("Fel");
    felButton->setButtonText("Flash Software");
    felButton->addListener(this);
    addAndMakeVisible(felButton);

    overlaySpinner = new OverlaySpinner();
    addChildComponent(overlaySpinner);

  buildName = "Build: ";
  auto releaseFileName = absoluteFileFromPath( "/etc/os-release" );
  File releaseFile( releaseFileName );
  if (releaseFile.exists()) {
    auto fileStr = releaseFile.loadFileAsString();
    auto lines = split(fileStr, "\n");
    if (lines.size() < 9)
      DBG(__func__ << ": No release information in /etc/os-release");
    else {
      auto releaseKv = split(lines[8],"=");
      std::vector<String> releaseV(releaseKv.begin()+1,releaseKv.end());
      for (const auto& val : releaseV) {
        // WIP: misses the removed equals
        buildName += val;
      }
      DBG(buildName);
    }
  }
  
#if JUCE_MAC
  buildName = "Build: MacOsX Dev Build";
#endif
  
  buildNameLabel = new Label("Build Name");
  buildNameLabel->setText(buildName, NotificationType::dontSendNotification);
  buildNameLabel->      setFont(16);
  buildNameLabel->setJustificationType(Justification::centred);
  addAndMakeVisible(buildNameLabel);
  
  //Create rev Text
  String rev_string = "Alpha v"+std::to_string(rev_number);
  if(bug_number != 0)
    rev_string += "."+std::to_string(bug_number);
  rev = new Label("rev", rev_string);
  addAndMakeVisible(rev);
  rev->setAlwaysOnTop(true);
  rev->setFont(Font(20.f));
  
  //Update window
  updateWindow = new AlertWindow("Checking for updates",
                   "Downloading informations, please wait...", 
                   AlertWindow::AlertIconType::NoIcon);
  addAndMakeVisible(updateWindow, 10);
  updateWindow->setAlwaysOnTop(true);
  updateWindow->setVisible(false);
}

PowerPageComponent::~PowerPageComponent() {}

void PowerPageComponent::hideLockscreen(){
    removeChildComponent(lockscreen);
    //Let's go back to the homescreen
    getMainStack().popPage(PageStackComponent::kTransitionNone);    
}

void PowerPageComponent::paint(Graphics &g) {
    auto bounds = getLocalBounds();
    g.fillAll(bgColor);
    g.drawImage(bgImage,bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), 0, 0, bgImage.getWidth(), bgImage.getHeight(), false);
}

void PowerPageComponent::resized() {
  
  auto bounds = getLocalBounds();
  overlaySpinner->setBounds(0, 0, bounds.getWidth(), bounds.getHeight());

  {
    unsigned int number = 4;
    
    for (int i = 0, j = 0; i < number; ++i) {
      if (i > 0) verticalLayout.setItemLayout(j++, 0, -1, -1);
      verticalLayout.setItemLayout(j++, 48, 48, 48);
    }

    Component *powerItems[] = { powerOffButton.get(), rebootButton.get(), sleepButton.get() };
    auto b = bounds.reduced(10);
    b.setLeft(70);
    verticalLayout.layOutComponents(powerItems, 1, b.getX(), b.getY(), b.getWidth(),
                                    b.getHeight(), true, true);
  }

  mainPage->setBounds(bounds);
  
  powerOffButton->setBounds(bounds.getWidth()/7, 40, bounds.getWidth() - 130, 40);
  sleepButton->setBounds(bounds.getWidth()/7, 90, bounds.getWidth() - 130, 40);
  rebootButton->setBounds(bounds.getWidth()/7, 140, bounds.getWidth() - 130, 40);
  felButton->setBounds(bounds.getWidth()/7, 190, bounds.getWidth() - 130, 40);
  backButton->setBounds(bounds.getWidth()-60, bounds.getY(), 60, bounds.getHeight());
  
  buildNameLabel->setBounds(bounds.getX(), bounds.getY(), bounds.getWidth(), 30);
  buildNameLabel->setBoundsToFit(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), Justification::centredBottom, true);
  rev->setBounds(bounds.getX(), bounds.getY(), 100, 30);
    
  int width = updateWindow->getWidth();
  int height = updateWindow->getHeight();
  int x = bounds.getWidth()/2-width/2;
  int y = bounds.getHeight()/2-height/2;
  updateWindow->setBounds(bounds.getX()+x, bounds.getY()+y, width, height);
}

void PowerPageComponent::setSleep() {
    #if JUCE_LINUX
    StringArray cmd{ "xset","q","|","grep","is O" };
    if(child.start(cmd)) {
        const String result (child.readAllProcessOutput());
        if( result == "Monitor is Off") {
            child.start("xset dpms force on");
        } else {
            lockscreen->hasPassword();
	    //Show the lockscreen
            addAndMakeVisible(lockscreen);
            lockscreen->setAlwaysOnTop(true);
	    //Turn off the screen
            child.start("xset dpms force off" );
        }
    }
  #endif
}

void PowerPageComponent::showPowerSpinner() {
    backButton->setVisible(false);
    powerOffButton->setVisible(false);
    sleepButton->setVisible(false);
    rebootButton->setVisible(false);
    felButton->setVisible(false);
    overlaySpinner->setVisible(true);
}

void PowerPageComponent::buttonStateChanged(Button *btn) {
  if (btn->isMouseButtonDown() && btn->isMouseOver()) {
    btn->setAlpha(0.5f);
  }
  else {
    btn->setAlpha(1.0f);
  }
}

void PowerPageComponent::buttonClicked(Button *button) {
  if (button == backButton) {
    getMainStack().popPage(PageStackComponent::kTransitionTranslateHorizontalLeft);
  } else if (button == powerOffButton) {
    showPowerSpinner();
    child.start("systemctl poweroff");
  } else if (button == rebootButton) {
    showPowerSpinner();
    child.start("systemctl reboot");
  } else if (button == sleepButton) {
    setSleep();
  } else if (button == felButton) {
    getMainStack().pushPage(felPage, PageStackComponent::kTransitionTranslateHorizontalLeft);
  }
}
