#include "AppsPageComponent.h"
#include "LauncherComponent.h"
#include "PokeLookAndFeel.h"
#include "Main.h"
#include "Utils.h"

using namespace std; 

void AppCheckTimer::timerCallback() {
  DBG("AppCheckTimer::timerCallback - check running apps");
  if (appsPage) {
    appsPage->checkRunningApps();
  }
}

void AppDebounceTimer::timerCallback() {
  DBG("AppDebounceTimer::timerCallback - check launch debounce");
  if (appsPage) {
    appsPage->debounce = false;
  }
  stopTimer();
}

AppIconButton::AppIconButton(const String &label, const String &shell, const Drawable *image)
: DrawableButton(label, DrawableButton::ImageAboveTextLabel),
  shell(shell) {
  // FIXME: supposedly setImages will "create internal copies of its drawables"
  // this relates to AppsPageComponent ownership of drawable icons ... docs are unclear
  setImages(image);
}

Rectangle<float> AppIconButton::getImageBounds() const {
  auto bounds = getLocalBounds();
  return bounds.withHeight(PokeLookAndFeel::getDrawableButtonImageHeightForBounds(bounds)).toFloat();
}

AppListComponent::AppListComponent(Component* parent, bool ishorizontal) :
  grid(ishorizontal ? new Grid(3, 2) : new Grid(2, 3)),
  direction(ishorizontal?HORIZONTAL:VERTICAL)
{
  string previcn = (direction==HORIZONTAL)?"backIcon.png":"pageUpIcon.png";
  string nexticn = (direction==HORIZONTAL)?"nextIcon.png":"pageDownIcon.png";
  prevPageBtn = createImageButton("PrevAppsPage",
                                  ImageFileFormat::loadFrom(assetFile(previcn)));
  nextPageBtn = createImageButton("NextAppsPage",
                                  ImageFileFormat::loadFrom(assetFile(nexticn)));

  
  if(!parent) parent = this;
  parent->addChildComponent(nextPageBtn);
  parent->addChildComponent(prevPageBtn, 500);
  NavigationListener* list = new NavigationListener(nextPageBtn, this);
  nextPageBtn->addListener(list);
  prevPageBtn->addListener(list);
  prevPageBtn->setAlwaysOnTop(true);
  
  addAndMakeVisible(grid);
}

AppListComponent::~AppListComponent() {}

void AppListComponent::next(){
    grid->showNextPage();
    checkShowPageNav();
}

void AppListComponent::previous(){
    grid->showPrevPage();
    checkShowPageNav();
}

DrawableButton *AppListComponent::createAndOwnIcon(const String &name, const String &iconPath, const String &shell) {
  File icon = assetFile(iconPath);
  Image image;
  if(iconPath == "" || !icon.exists() || icon.getSize()==0)
    image = createImageFromFile(assetFile("appIcons/default.png"));
  else
    image = createImageFromFile(icon);
  auto drawable = new DrawableImage();
  drawable->setImage(image);
  // FIXME: is this OwnedArray for the drawables actually necessary?
  // won't the AppIconButton correctly own the drawable?
  // Further we don't actually use this list anywhere.
  iconDrawableImages.add(drawable);
  auto button = new AppIconButton(name, shell, drawable);
  addAndOwnIcon(name, button);
  return button;
}

void AppListComponent::resized() {
  auto b = getLocalBounds();
  
  prevPageBtn->setSize(btnHeight, btnHeight);
  nextPageBtn->setSize(btnHeight, btnHeight);
  if(direction == HORIZONTAL){
    prevPageBtn->setBoundsToFit(b.getX(), b.getY(), b.getWidth(), b.getHeight(), Justification::centredLeft, true);
    nextPageBtn->setBoundsToFit(b.getX(), b.getY(), b.getWidth(), b.getHeight(), Justification::centredRight, true);

  }else if(direction == VERTICAL){
    
    prevPageBtn->setBoundsToFit(b.getX(), b.getY(), b.getWidth(), b.getHeight(), Justification::centredTop, true);
    nextPageBtn->setBoundsToFit(b.getX(), b.getY(), b.getWidth(), b.getHeight(), Justification::centredBottom, true);

  }
  // drop the page buttons from our available layout size
  auto gridWidth = b.getWidth() - (2.0*btnHeight);
  auto gridHeight = b.getHeight() - (2.0*btnHeight);
  grid->setSize(gridWidth, gridHeight);
  grid->setBoundsToFit(b.getX(), b.getY(), b.getWidth(), b.getHeight(), Justification::centred, true);
  
}

void AppListComponent::checkShowPageNav() {
  if (grid->hasNextPage()) {
    nextPageBtn->setVisible(true); nextPageBtn->setEnabled(true);
  }
  else {
    nextPageBtn->setVisible(false); nextPageBtn->setEnabled(false);
  }
  
  if (grid->hasPrevPage()) {
    prevPageBtn->setVisible(true); prevPageBtn->setEnabled(true);
  }
  else {
    prevPageBtn->setVisible(false); prevPageBtn->setEnabled(false);
  }
}

void AppListComponent::addAndOwnIcon(const String &name, Component *icon) {
  gridIcons.add(icon);
  grid->addItem(icon);
  ((Button*)icon)->addListener(this);
  ((Button*)icon)->addMouseListener(this, false);
}

void AppListComponent::removeIcon(Component* icon){
  gridIcons.removeObject(icon);
  grid->removeItem(icon);
}

Array<DrawableButton *> AppListComponent::createIconsFromJsonArray(const var &json) {
  Array<DrawableButton *> buttons;
  if (json.isArray()) {
    for (const auto &item : *json.getArray()) {
      auto name = item["name"];
      auto shell = item["shell"];
      auto iconPath = item["icon"];
      if (name.isString() && shell.isString() && iconPath.isString()) {
        auto icon = createAndOwnIcon(name, iconPath, shell);
        if (icon) {
          buttons.add(icon);
        }
      }
    }
  }
  
  checkShowPageNav();
  return buttons;
}

AppsPageComponent::AppsPageComponent(LauncherComponent* launcherComponent, bool horizontal) :
  AppListComponent(launcherComponent, horizontal),
  launcherComponent(launcherComponent),
  runningCheckTimer(),
  debounceTimer(),
  x(-1), y(-1), shouldMove(false)
{
  runningCheckTimer.appsPage = this;
  debounceTimer.appsPage = this;
  cpy = nullptr;
  
  //Trash Icon
  trashButton = createImageButton(
    "Trash", createImageFromFile(assetFile("trash.png")));
  trashButton->setName("Trash");
  trashButton->setAlwaysOnTop(true);
  addAndMakeVisible(trashButton, 100);
  trashButton->setBounds(170, 15, 40, 20);
  trashButton->setVisible(false);
  // Focus keyboard needed for the key listener
  setWantsKeyboardFocus(true);
}

AppsPageComponent::~AppsPageComponent() {}

Array<DrawableButton *> AppsPageComponent::createIconsFromJsonArray(const var &json) {
  auto buttons = AppListComponent::createIconsFromJsonArray(json);
  
  //// hard coded "virtual" application. Cannot be removed.
  //appsLibraryBtn = createAndOwnIcon("App Get", "appIcons/update.png", String::empty);
  //buttons.add(appsLibraryBtn);
  
  checkShowPageNav();
  return buttons;
}

void AppsPageComponent::startApp(AppIconButton* appButton) {
  DBG("AppsPageComponent::startApp - " << appButton->shell);
  ChildProcess* launchApp = new ChildProcess();
  launchApp->start("xmodmap ${HOME}/.Xmodmap"); // Reload xmodmap to ensure it's running
  if (launchApp->start(appButton->shell)) {
    runningApps.add(launchApp);
    runningAppsByButton.set(appButton, runningApps.indexOf(launchApp));
    // FIXME: uncomment when process running check works
    // runningCheckTimer.startTimer(5 * 1000);
    
    debounce = true;
    debounceTimer.startTimer(2 * 1000);
    
    // TODO: should probably put app button clicking logic up into LauncherComponent
    // correct level for event handling needs more thought
    launcherComponent->showLaunchSpinner();
  }
};

void AppsPageComponent::focusApp(AppIconButton* appButton, const String& windowId) {
  DBG("AppsPageComponent::focusApp - " << appButton->shell);
  String focusShell = "echo 'focus_client_by_window_id("+windowId+")' | awesome-client";
  StringArray focusCmd{"sh", "-c", focusShell.toRawUTF8()};
  ChildProcess focusWindow;
  focusWindow.start(focusCmd);
};

void AppsPageComponent::startOrFocusApp(AppIconButton* appButton) {
  if (debounce) return;
  
  bool shouldStart = true;
  int appIdx = runningAppsByButton[appButton];
  bool hasLaunched = runningApps[appIdx] != nullptr;
  String windowId;
  
  if(hasLaunched) {
    const auto shellWords = split(appButton->shell, " ");
    const auto& cmdName = shellWords[0];
    StringArray findCmd{"xdotool", "search", "--all", "--limit", "1", "--class", cmdName.toRawUTF8()};
    ChildProcess findWindow;
    findWindow.start(findCmd);
    findWindow.waitForProcessToFinish(1000);
    windowId = findWindow.readAllProcessOutput().trimEnd();
    
    // does xdotool find a window id? if so, we shouldn't start a new one
    shouldStart = (windowId.length() > 0) ? false : true;
  }
  
  if (shouldStart) {
    startApp(appButton);
  }
  else {
    focusApp(appButton, windowId);
  }
  
};

void AppsPageComponent::openAppsLibrary() {
  launcherComponent->showAppsLibrary();
}

void AppsPageComponent::checkRunningApps() {
  Array<int> needsRemove{};
  
  // check list to mark any needing removal
  for (const auto& cp : runningApps) {
    if (!cp->isRunning()) {
      needsRemove.add(runningApps.indexOf(cp));
    }
  }
  
  // cleanup list
  for (const auto appIdx : needsRemove) {
    runningApps.remove(appIdx);
    runningAppsByButton.removeValue(appIdx);
  }
  
  if (!runningApps.size()) {
    // FIXME: uncomment when process running check works
    // runningCheckTimer.stopTimer();
    launcherComponent->hideLaunchSpinner();
  }
};

void AppsPageComponent::buttonStateChanged(Button* btn) {
  AppIconButton* appBtn = (AppIconButton*)btn;
  DrawableImage* appIcon = (DrawableImage*)appBtn->getCurrentImage();
  auto buttonPopup = launcherComponent->focusButtonPopup.get();
  constexpr auto scale = 1.3;

  // show floating button popup if we're holding downstate and not showing the popup
  if (btn->isMouseButtonDown() &&
      btn->isMouseOver() &&
      !buttonPopup->isVisible()) {
    // copy application icon bounds in screen space
    auto boundsNext = appIcon->getScreenBounds();
    auto boundsCentre = boundsNext.getCentre();
    
    // scale and recenter
    boundsNext.setSize(boundsNext.getWidth()*scale, boundsNext.getHeight()*scale);
    boundsNext.setCentre(boundsCentre);
    
    // translate back to space local to popup parent (local bounds)
    auto parentPos = launcherComponent->getScreenPosition();
    boundsNext.setPosition(boundsNext.getPosition() - parentPos);
    
    // show popup icon, hide real button beneath
    buttonPopup->setImage(appIcon->getImage());
    buttonPopup->setBounds(boundsNext);
    buttonPopup->setVisible(true);
    appIcon->setVisible(false);
    appBtn->setColour(DrawableButton::textColourId, Colours::transparentWhite);
  }
  // set UI back to default if we can see the popup, but aren't holding the button down
  else if (btn->isVisible()) {
    appIcon->setVisible(true);
    appBtn->setColour(DrawableButton::textColourId, getLookAndFeel().findColour(DrawableButton::textColourId));
    buttonPopup->setVisible(false);
  }
}

void AppsPageComponent::mouseDrag(const MouseEvent& me){
  if(me.originalComponent == this ||
     me.getLengthOfMousePress() < 500) return;
  launcherComponent->setIpVisible(false);
  
  //Get the position of the mouse relative to the Grid
  Point<int> pi = me.getPosition();
  Point<int> res = this->getLocalPoint(nullptr, me.getScreenPosition());
  if(cpy==nullptr){
    DrawableButton* dragging = (DrawableButton* const) me.originalComponent;
    Drawable* img = dragging->getNormalImage();
    cpy = img->createCopy();
    cpy->setOriginWithOriginalSize(Point<float>(0.f,0.f));
    addAndMakeVisible(cpy);
  }
  int drag_x = res.x - cpy->getWidth()/2;
  int drag_y = res.y - cpy->getHeight()/2;
  trashButton->setVisible(true);

  //x==-1 means this is the first first the function is called
  //So a new icon has just been pressed
  if(x==-1 && y==-1){
    x = drag_x;
    y = drag_y;
    //We draw the dragging icon at this coordinates
    cpy->setBounds(x, y, cpy->getWidth(), cpy->getHeight());
  }
  
  //The icon should move only if it has been dragged more than 5 pixels
  shouldMove = abs(x-drag_x)>5 || abs(y-drag_y)>5;

  //Should it move ?
  if(shouldMove){
    //Let's move it !
    cpy->setBounds(drag_x, drag_y, cpy->getWidth(), cpy->getHeight());
    x = drag_x;
    y = drag_y;
  }
  
  /* If the mouse is on the top of the screen (less than 10px)
   * The icon because more transparent
   */
  if(drag_y <= 10) cpy->setAlpha(0.3);
  else cpy->setAlpha(0.9);
}

void AppsPageComponent::onTrash(Button* button){
    bool answer = AlertWindow::showOkCancelBox(AlertWindow::AlertIconType::WarningIcon,
                                  "Delete icon ?", 
                                  "Are you sure you want to delete "+button->getName()+" ?",
                                  "Yes",
                                  "No"
                                 );
    if(answer){
        auto appButton = (AppIconButton*) button;
        launcherComponent->deleteIcon(button->getName(), appButton->shell, appButton);
    }
}

void AppsPageComponent::buttonClicked(const MouseEvent& me){
  if(me.originalComponent == this) return;
  Button* button = (Button*) me.originalComponent;
  ModifierKeys mk = ModifierKeys::getCurrentModifiers();

  if (mk.isCtrlDown() ||
      me.mods.isRightButtonDown()
     ){
    //Control key was pressed
    PopupMenu pop;
    pop.addItem(EDIT, "Edit");
    pop.addItem(MOVELEFT, "Move left");
    pop.addItem(MOVERIGHT, "Move right");
    pop.addItem(DELETE, "Delete");
    int choice = pop.show();
    //If nothing has been selected then... do nothing
    if(!choice) return;
    manageChoice((AppIconButton*) button, choice);
  }
  else{
    auto appButton = (AppIconButton*)button;
    startOrFocusApp(appButton);
  }
}

bool AppsPageComponent::keyPressed(const KeyPress& key){
  const int code = key.getKeyCode();
  const int up = KeyPress::upKey;
  const int down = KeyPress::downKey;
  const int left = KeyPress::leftKey;
  const int right = KeyPress::rightKey;
  const int enter = KeyPress::returnKey;

  bool newpage = false;
  if(code == up){
    /* Case Up */
    newpage = grid->selectPrevious(grid->numCols);
  } else if (code == down) {
    /* Case Down */
    newpage = grid->selectNext(grid->numCols);
  } else if (code == left) {
    /* Case Left */
    newpage = grid->selectPrevious();
  } else if (code == right) {
    /* Case Right */
    newpage = grid->selectNext();
  } else if (code == enter) {
    AppIconButton* button = (AppIconButton*) grid->getSelected();
    startOrFocusApp(button);    
  } else {
    return false;
  }
  if(newpage) checkShowPageNav();
  return true;
}

void AppsPageComponent::mouseUp(const MouseEvent& me){
  if(!cpy){
    buttonClicked(me);
    return;
  }
  bool ontrash = cpy->getAlpha()>=0.25 && cpy->getAlpha()<=0.35;
  if(ontrash){
    //On Delete icon
    Button* button = (Button*) me.originalComponent;
    onTrash(button);
  }
  trashButton->setVisible(false);
  removeChildComponent(cpy);
  cpy = nullptr;
  if(me.getLengthOfMousePress() < 1000 && !ontrash)
    buttonClicked((Button*) me.originalComponent);
  launcherComponent->setIpVisible(true);
  //Setting back "old values" to -1
  x = -1;
  y = -1;
  shouldMove = false;
}

/* left is true if the current icon has to be switched 
 * with the one on its left */
void AppsPageComponent::moveInConfig(AppIconButton* icon, bool left){
  //Get the global configuration
  var json = getConfigJSON();
  Array<var>* pages_arr = (json["pages"].getArray());
  const var& pages = ((*pages_arr)[0]);
  Array<var>* items_arr = pages["items"].getArray();
  
  //Offset of the icon to switch with
  int nxtoffset = left?-1:1;
  //Searching for the element in the Array
  for(int i = 0; i < items_arr->size(); i++){
    const var& elt = (*items_arr)[i];
    if(elt["name"] == icon->getName() && elt["shell"] == icon->shell){
      items_arr->swap(i, i+nxtoffset);
      break;
    }
  }
  
  //Write the config object to the file
  File config = getConfigFile();
  DynamicObject* obj = json.getDynamicObject();
  String s = JSON::toString(json);
  config.replaceWithText(s);
}

void AppsPageComponent::updateIcon(AppIconButton* icon, EditWindow* ew){
  //Get the global configuration
  var json = getConfigJSON();
  Array<var>* pages_arr = (json["pages"].getArray());
  const var& pages = ((*pages_arr)[0]);
  Array<var>* items_arr = pages["items"].getArray();
  
  //Searching for the element in the Array
  for(int i = 0; i < items_arr->size(); i++){
    const var& elt = (*items_arr)[i];
    if(elt["name"] == icon->getName() && elt["shell"] == icon->shell){
      DynamicObject* new_elt = elt.getDynamicObject();
      new_elt->setProperty("name",  ew->getName());
      icon->setName(ew->getName());
      new_elt->setProperty("shell", ew->getShell());
      icon->shell = ew->getShell();
      if(ew->getIcon() != ""){
	new_elt->setProperty("icon", ew->getIcon());
	Image img = createImageFromFile(ew->getIcon());
	DrawableImage newicn;
	newicn.setImage(img);
	icon->setImages(&newicn);
      }
      break;
    }
  }
  
  //Write the config object to the file
  File config = getConfigFile();
  DynamicObject* obj = json.getDynamicObject();
  String s = JSON::toString(json);
  config.replaceWithText(s);
}

void AppsPageComponent::manageChoice(AppIconButton* icon, int choice){
  EditWindow* ew;
  auto b = getLocalBounds();
  
  bool answer;
  switch(choice){
    case EDIT:
      ew = new EditWindow(icon, b.getWidth(), b.getHeight());
      addAndMakeVisible(ew);
      answer = ew->invoke();
      if(answer) updateIcon(icon, ew);
      //Process result here, then delete
      removeChildComponent(ew);
      delete ew;
      break;
      
    case MOVELEFT:
      //Moving for the current session
      answer = grid->moveLeft(icon);
      //Moving for the next session (update config.json)
      if(answer) moveInConfig(icon, true);
      break;
      
    case MOVERIGHT:
      //Moving for the current session
      answer = grid->moveRight(icon);
      //Moving for the next session (update config.json)
      if(answer) moveInConfig(icon, false);
      break;
      
    case DELETE:
      onTrash(icon);
      break;

    default:
      break;
  }
}

void AppsPageComponent::buttonClicked(Button *button) {  
  if (button == prevPageBtn) {
    grid->showPrevPage();
    checkShowPageNav();
  }
  else if (button == nextPageBtn) {
    grid->showNextPage();
    checkShowPageNav();
  }
  else if (button == appsLibraryBtn) {
    openAppsLibrary();
  }
  /*else if (mk.isCtrlDown()){
    //Control key was pressed
    PopupMenu pop;
    pop.addItem(EDIT, "Edit");
    pop.addItem(MOVELEFT, "Move left");
    pop.addItem(MOVERIGHT, "Move right");
    pop.addItem(DELETE, "Delete");
    int choice = pop.show();
    //If nothing has been selected then... do nothing
    if(!choice) return;
    manageChoice((AppIconButton*) button, choice);
  }
  else{
    auto appButton = (AppIconButton*)button;
    startOrFocusApp(appButton);
  }*/
}

NavigationListener::NavigationListener(Button* next, AppListComponent* p): next(next), page(p){}

void NavigationListener::buttonClicked(Button *button){
    if(button==next) page->next();
    else page->previous();
}

EditWindow::EditWindow(AppIconButton* button, int w, int h):
button(button),
lname("name", "Name: "), licon("icon", "Icon path: "), 
lshell("shell", "Shell command: "),
apply("Apply", "Apply"), cancel("Cancel"),
browse("..."), choice(0)
{
  browse.addListener(this);
  apply.addListener(this);
  cancel.addListener(this);
  
  //Set up the text fields
  name.setText(button->getName());
  shell.setText(button->shell);
  
  lname.setColour(Label::ColourIds::textColourId, Colours::black);
  licon.setColour(Label::ColourIds::textColourId, Colours::black);
  lshell.setColour(Label::ColourIds::textColourId, Colours::black);
  
  int size = w;
  int gap = 30;
  int sizebrowse = 35;
  this->setBounds(0, 30, size, h);
  
  int width = (size-150)/2;
  apply.setBounds(gap, 172, width, 30);
  cancel.setBounds(2*gap+(size/2), 172, width, 30);
  lname.setBounds(gap, 10, width, 30);
  licon.setBounds(gap, 50, width, 30);
  lshell.setBounds(gap, 90, width, 30);
  
  //Placing the text fields
  int x = gap+width+gap;
  name.setBounds(x, 10, size/2, 30);
  icon.setBounds(x, 50, size/2-sizebrowse, 30);
  shell.setBounds(x,90, size/2, 30);
  //Browsing icon
  browse.setBounds(x+size/2-sizebrowse, 50, sizebrowse, 30);
  
  addAndMakeVisible(lname);
  addAndMakeVisible(licon);
  addAndMakeVisible(lshell);
  addAndMakeVisible(name);
  addAndMakeVisible(icon);
  addAndMakeVisible(shell);
  addAndMakeVisible(apply);
  addAndMakeVisible(cancel);
  addAndMakeVisible(browse);
  
  //Looking for the image path
  var json = getConfigJSON();
  Array<var>* pages_arr = (json["pages"].getArray());
  const var& pages = ((*pages_arr)[0]);
  Array<var>* items_arr = pages["items"].getArray();
  
  //Searching for the element in the Array
  for(int i = 0; i < items_arr->size(); i++){
    const var& elt = (*items_arr)[i];
    if(elt["name"] == button->getName() && elt["shell"] == button->shell){
      icon.setText(elt["icon"]);
      break;
    }
  }
}

EditWindow::~EditWindow(){ }

void EditWindow::paint(Graphics &g){
  g.fillAll(Colour(0xFFEDEDED));
}

void EditWindow::mouseDown(const MouseEvent &event){
}

void EditWindow::buttonClicked(Button* button){
  if(button == &apply)
    choice = true;
  else if(button == &cancel)
    choice = false;
  else{
    WildcardFileFilter wildcardFilter ("*.png;*.jpg;*.jpeg", 
                                       String::empty,
                                       "Image files");

    FileBrowserComponent browser (FileBrowserComponent::canSelectFiles |
                                  FileBrowserComponent::openMode,
                                  File::nonexistent,
                                  &wildcardFilter,
                                  nullptr);

    FileChooserDialogBox dialogBox ("Choose the icon",
                                    "Please choose your png icon (ideal size : 90x70 px)",
                                    browser,
                                    false,
                                    Colours::lightgrey);

    auto b = getLocalBounds();
    if(dialogBox.show(b.getWidth(),b.getHeight())){
      File selectedFile = browser.getSelectedFile(0);
      String path = selectedFile.getFullPathName();
      icon.setText(path);
    }
    return;
  }
  exitModalState(0);
}

void EditWindow::resized(){
  auto bounds = this->getBounds();
}

bool EditWindow::invoke(){
  this->setVisible(true);
  runModalLoop();
  return choice;
}

String EditWindow::getName(){
  return name.getText();
}

String EditWindow::getIcon(){
  return icon.getText();
}

String EditWindow::getShell(){
  return shell.getText();
}
