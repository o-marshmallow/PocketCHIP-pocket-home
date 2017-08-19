#include "LauncherComponent.h"
#include "AppsPageComponent.h"
#include "LibraryPageComponent.h"
#include "SettingsPageComponent.h"
#include "PowerPageComponent.h"

#include "Main.h"
#include "Utils.h"
#include <math.h>
#include <algorithm>

void LaunchSpinnerTimer::timerCallback() {
  if (launcherComponent) {
    auto lsp = launcherComponent->launchSpinner.get();
    const auto& lspImg = launcherComponent->launchSpinnerImages;
    
    // change image
    i++;
    if (i == lspImg.size()) { i = 0; }
    lsp->setImage(lspImg[i]);
    
    // check timeout
    t += getTimerInterval();
    if (t > timeout) {
      t = 0;
      lsp->setVisible(false);
      stopTimer();
    }
  }
}

void BatteryIconTimer::timerCallback() {
  
  // get current battery status from the battery monitor thread
  auto batteryStatus = launcherComponent->batteryMonitor.getCurrentStatus();
  
  // we can't change anything if we don't have a LauncherComponent
  if(launcherComponent) {
    
    // we want to modify the "Battery" icon
      const auto& batteryIcons = launcherComponent->batteryIconImages;
      const auto& batteryIconsCharging = launcherComponent->batteryIconChargingImages;
      

    for( auto button : launcherComponent->topButtons->buttons ) {
      Image batteryImg = batteryIcons[3];
      if (button->getName() == "Battery") {
          int status = round( ((float)batteryStatus.percentage)/100.0f * 3.0f );
          
          int pct = (int) batteryStatus.percentage;
          juce::String pct_s = std::to_string(pct)+" %";
          launcherComponent->batteryLabel->setText(pct_s, dontSendNotification);
          
          if( batteryStatus.percentage <= 5 ) {
              status = 3;
          } else {
              // limit status range to [0:3]
              if(status < 0) status = 0;
              if(status > 2) status = 2;
          }
          if( !batteryStatus.isCharging ) {
              batteryImg = batteryIcons[status];
          } else {
              batteryImg = batteryIconsCharging[status];

          }
          
          button->setImages(false, false, true,
                       batteryImg, 1.0f, Colours::transparentWhite, // normal
                       batteryImg, 1.0f, Colours::transparentWhite, // over
                       batteryImg, 0.5f, Colours::transparentWhite, // down
                       0);
      }
    }
  }
  
  //DBG( "Charging: "  << batteryStatus.isCharging );
  //DBG( "Voltage: " << batteryStatus.percentage );
  
}

void WifiIconTimer::timerCallback() {
  if(!launcherComponent) { return; }
    
  for( auto button : launcherComponent->topButtons->buttons ) {
    if (button->getName() == "WiFi") {
      Image wifiIcon;
      const auto& conAp = getWifiStatus().connectedAccessPoint();
      
      // wifi on and connected
      if (getWifiStatus().isConnected() && conAp) {
        //Get IP and show it  
        launcherComponent->updateIp();
        // 0 to 100
        float sigStrength = std::max(0, std::min(99, conAp->signalStrength));
        // don't include the wifi-off icon as a bin
        int iconBins = launcherComponent->wifiIconImages.size() - 2;
        int idx = round( ( iconBins * (sigStrength)/100.0f) );
        DBG(__func__ << ": accessing icon " << idx);
        wifiIcon = launcherComponent->wifiIconImages[idx];
      }
      // wifi on but no connection
      else if (getWifiStatus().isEnabled()) {
        wifiIcon = launcherComponent->wifiIconImages[0];
        launcherComponent->setIpVisible(false);
      }
      // wifi off
      else {
        wifiIcon = launcherComponent->wifiIconImages.getLast();
        launcherComponent->setIpVisible(false);
      }
      
      button->setImages(false, false, true,
                        wifiIcon, 1.0f, Colours::transparentWhite, // normal
                        wifiIcon, 1.0f, Colours::transparentWhite, // over
                        wifiIcon, 0.5f, Colours::transparentWhite, // down
                        0);
    }
  }
}

void LauncherComponent::setColorBackground(const String& str){
    String value = "FF" + str;
    unsigned int x;   
    std::stringstream ss;
    ss << std::hex << value;
    ss >> x;
    bgColor = Colour(x);
    hasImg = false;
}

void LauncherComponent::setImageBackground(const String& str){
    String value(str);
    if(value == "") value = "mainBackground.png";
    File f;
    if(value[0]=='~' || value[0]=='/') f = File(value);
    else f = assetFile(value);
    bgImage = createImageFromFile(f);
    hasImg = true;
}

void LauncherComponent::setClockVisible(bool visible){
  if(visible){
    if(clock->isThreadRunning()) return;
    addAndMakeVisible(clock->getLabel(), 10);
    clock->startThread();
  }else{
    if(!clock->isThreadRunning()) return;
    Label& l = clock->getLabel();
    removeChildComponent(&l);
    clock->stopThread(1500);
  }
}

void LauncherComponent::setClockAMPM(bool ampm){
  clock->setAmMode(ampm);
}

LauncherComponent::LauncherComponent(const var &configJson) :
clock(nullptr), labelip("ip", "")
{
  /* Ip settings */
  labelip.setVisible(false);

  /* Setting the clock */
  clock = new ClockMonitor;
  clock->getLabel().setBounds(380, 0, 50, 50);
  String displayclock = (configJson["showclock"]).toString();
  String formatclock = (configJson["timeformat"]).toString();
  if(displayclock.length()==0 || displayclock==String("yes"))
    setClockVisible(true);
  else
    setClockVisible(false);
  
  setClockAMPM(formatclock == "ampm");
  
  /* Battery percentage label */
  batteryLabel = new Label("percentage", "-%");
  addAndMakeVisible(batteryLabel);
  batteryLabel->setFont(Font(15.f));
//   batteryLabel->setOpaque(false);
//   batteryLabel->setAlwaysOnTop(true);
//   batteryLabel->addToDesktop(ComponentPeer::StyleFlags::windowIsSemiTransparent);
  
  String value = (configJson["background"]).toString();
  
  bgColor = Colour(0x4D4D4D);
  if(value.length()==6 && value.containsOnly("0123456789ABCDEF"))
    setColorBackground(value);
  else
    setImageBackground(value);
  
  pageStack = new PageStackComponent();
  addAndMakeVisible(pageStack);

  topButtons = new LauncherBarComponent();
  botButtons = new LauncherBarComponent();
  topButtons->setInterceptsMouseClicks(false, true);
  botButtons->setInterceptsMouseClicks(false, true);
  addAndMakeVisible(topButtons);
  addAndMakeVisible(botButtons);
  
  Array<String> wifiImgPaths{"wifiStrength0.png","wifiStrength1.png","wifiStrength2.png","wifiStrength3.png","wifiOff.png"};
  for(auto& path : wifiImgPaths) {
    auto image = createImageFromFile(assetFile(path));
    wifiIconImages.add(image);
  }
  
  Array<String> batteryImgPaths{"battery_1.png","battery_2.png","battery_3.png","battery_0.png"};
  for(auto& path : batteryImgPaths) {
    auto image = createImageFromFile(assetFile(path));
    batteryIconImages.add(image);
  }
    
  Array<String> batteryImgChargingPaths{"batteryCharging_1.png","batteryCharging_2.png","batteryCharging_3.png","batteryCharging_0.png"};
  for(auto& path : batteryImgChargingPaths) {
    auto image = createImageFromFile(assetFile(path));
    batteryIconChargingImages.add(image);
  }

  launchSpinnerTimer.launcherComponent = this;
  Array<String> spinnerImgPaths{"wait0.png","wait1.png","wait2.png","wait3.png","wait4.png","wait5.png","wait6.png","wait7.png"};
  for(auto& path : spinnerImgPaths) {
    auto image = createImageFromFile(assetFile(path));
    launchSpinnerImages.add(image);
  }
  
  launchSpinner = new ImageComponent();
  launchSpinner->setImage(launchSpinnerImages[0]);
  launchSpinner->setInterceptsMouseClicks(false, false);
  addChildComponent(launchSpinner);
  
  focusButtonPopup = new ImageComponent("Focus Button Popup");
  focusButtonPopup->setInterceptsMouseClicks(false, false);
  addChildComponent(focusButtonPopup);
  
  // Settings page
  auto settingsPage = new SettingsPageComponent(this);
  settingsPage->addChildComponent(labelip);
  settingsPage->setName("Settings");
  pages.add(settingsPage);
  pagesByName.set("Settings", settingsPage);
  
  // Power page
  auto powerPage = new PowerPageComponent();
  powerPage->setName("Power");
  pages.add(powerPage);
  pagesByName.set("Power", powerPage);
  
  // Apps page
  /* Check whether we have to display vertically the icons
   * Default based on window size, but allow override in config
   */
  auto b = getLocalBounds();
  bool vertical =
    (configJson["direction"].toString()=="" && b.getWidth() < b.getHeight()) ||
    (configJson["direction"].toString()=="VERTICAL");
  auto appsPage = new AppsPageComponent(this, !vertical);
  appsPage->setName("Apps");
  pages.add(appsPage);
  pagesByName.set("Apps", appsPage);
  
  // Apps library
  auto appsLibrary = new LibraryPageComponent();
  appsLibrary->setName("AppsLibrary");
  pages.add(appsLibrary);
  pagesByName.set("AppsLibrary", appsLibrary);
  
  // Read config for apps and corner locations
  auto pagesData = configJson["pages"].getArray();
  if (pagesData) {
    for (const auto &page : *pagesData) {
      auto name = page["name"].toString();
      if (name == "Apps") {
        
        const auto& appButtons = appsPage->createIconsFromJsonArray(page["items"]);
        for (auto button : appButtons) { button->setWantsKeyboardFocus(false); }
        appsLibrary->createIconsFromJsonArray(page["items"]);
        auto buttonsData = *(page["cornerButtons"].getArray());
        
        // FIXME: is there a better way to slice juce Array<var> ?
        Array<var> topData{};
        Array<var> botData{};
        topData.add(buttonsData[0]);
        topData.add(buttonsData[1]);
        botData.add(buttonsData[2]);
        botData.add(buttonsData[3]);
        
        topButtons->addButtonsFromJsonArray(topData);
        botButtons->addButtonsFromJsonArray(botData);
      }
    }
  }
  
  // NOTE(ryan): Maybe do something with a custom event later.. For now we just listen to all the
  // buttons manually.
  for (auto button : topButtons->buttons) {
    button->setWantsKeyboardFocus(false);
    button->setInterceptsMouseClicks(false, false);
  }
  for (auto button : botButtons->buttons) {
    button->addListener(this);
    button->setWantsKeyboardFocus(false);
  }

  defaultPage = pagesByName[configJson["defaultPage"]];
  
  batteryMonitor.updateStatus();
  batteryMonitor.startThread();
  
  batteryIconTimer.launcherComponent = this;
  batteryIconTimer.startTimer(1000);
  batteryIconTimer.timerCallback();
  
  wifiIconTimer.launcherComponent = this;
  wifiIconTimer.startTimer(2000);
  wifiIconTimer.timerCallback();

}

LauncherComponent::~LauncherComponent() {
  batteryIconTimer.stopTimer();
  batteryMonitor.stopThread(2000);
}

void LauncherComponent::paint(Graphics &g) {
  auto bounds = getLocalBounds();
  g.fillAll(bgColor);
  if(hasImg) g.drawImage(bgImage,bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), 0, 0, bgImage.getWidth(), bgImage.getHeight(), false);
  //g.drawImage(trashButton, bounds.getX()+395, bounds.getY()+16, 40, 20, 0, 0, 50, 50, false);
}

void LauncherComponent::resized() {
  auto bounds = getLocalBounds();
  int barSize = 50;
  
  topButtons->setBounds(bounds.getX(), bounds.getY(), bounds.getWidth(),
                        barSize);
  botButtons->setBounds(bounds.getX(), bounds.getHeight() - barSize, bounds.getWidth(),
                             barSize);
  pageStack->setBounds(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight());
  launchSpinner->setBounds(0, 0, bounds.getWidth(), bounds.getHeight());
  
  batteryLabel->setBounds(bounds.getX()+40, bounds.getY(), 50, 50);
  
  clock->getLabel().setBounds(bounds.getX()+370, bounds.getY(), 80, 50);

  labelip.setBounds(bounds.getX()+190, bounds.getY(), 100, 30);
  // init
  if (!resize) {
    resize = true;
    pageStack->swapPage(defaultPage, PageStackComponent::kTransitionNone);
  }
}

void LauncherComponent::updateIp(){
  //Creating a socket
  int fd = socket(AF_INET, SOCK_DGRAM, 0);

  //This will help us getting the IPv4 associated with wlan0 interface
  struct ifreq ifr;
  memset(&ifr, 0, sizeof(ifr));
  ifr.ifr_addr.sa_family = AF_INET;
  //Copying the string "wlan0" in the structure
  sprintf(ifr.ifr_name, "wlan0");
  //Getting the informations of the socket, so IP address
  ioctl(fd, SIOCGIFADDR, &ifr);
  close(fd);

  char* addr = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
  String ip(addr);
  //Showing the new ip if different than 0.0.0.0
  if(addr == "0.0.0.0"){
    labelip.setVisible(false);
    return;
  }
  labelip.setText("IP: "+ip, dontSendNotification);
  labelip.setVisible(true);
}

void LauncherComponent::setIpVisible(bool v){
  labelip.setVisible(v);
}

void LauncherComponent::addIcon(const String& name, const String& path, const String& shell){
  AppsPageComponent* appsPage = (AppsPageComponent*) pagesByName["Apps"];
  DrawableButton* db = appsPage->createAndOwnIcon(name, path, shell);
  db->setWantsKeyboardFocus(false);
  appsPage->grid->showCurrentPage();
  appsPage->checkShowPageNav();
}

void LauncherComponent::showLaunchSpinner() {
  DBG("Show launch spinner");
  launchSpinner->setVisible(true);
  launchSpinnerTimer.startTimer(500);
}

void LauncherComponent::hideLaunchSpinner() {
  DBG("Hide launch spinner");
  launchSpinnerTimer.stopTimer();
  launchSpinner->setVisible(false);
}

void LauncherComponent::showAppsLibrary() {
  getMainStack().pushPage(pagesByName["AppsLibrary"], PageStackComponent::kTransitionTranslateHorizontalLeft);
}

void LauncherComponent::buttonClicked(Button *button) {
  auto currentPage = pageStack->getCurrentPage();
  
  if ((!currentPage || currentPage->getName() != button->getName()) &&
      pagesByName.contains(button->getName())) {
    auto page = pagesByName[button->getName()];
    if (button->getName() == "Settings") {
      getMainStack().pushPage(page, PageStackComponent::kTransitionTranslateHorizontal);
    } else if (button->getName() == "Power") {
        getMainStack().pushPage(page, PageStackComponent::kTransitionTranslateHorizontalLeft);
    } else {
      pageStack->swapPage(page, PageStackComponent::kTransitionTranslateHorizontal);
    }
  }
}

void LauncherComponent::deleteIcon(String name, String shell, Component* button){
  SettingsPageComponent* system = (SettingsPageComponent*) pagesByName["Settings"];
  system->deleteIcon(name,shell);
  /* Deleting graphically, without rebooting the app */
  AppsPageComponent* appsPage = (AppsPageComponent*) pagesByName["Apps"];
  appsPage->removeIcon(button);
}
