#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

class GridPage : public Component {
public:
  GridPage(int numCols, int numRows);
  
  int numCols;
  int numRows;
  
  bool addItem(Component *item);
  bool removeItem(Component*);
  void resized() override;
  bool moveRight(Component*);
  bool moveLeft(Component*);
  void switchLastToFirst(GridPage*);

  Array<Component *> items;
  OwnedArray<Component> gridRows;
  
private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GridPage)

};

class Grid : public Component {
public:
  Grid(int numCols, int numRows);
  
  int numCols;
  int numRows;
  Array<Component *> items;
  OwnedArray<GridPage> pages;
  GridPage* page = nullptr;
  
  void createPage();
  void addItem(Component *item);
  void removeItem(Component*);
  bool hasPrevPage();
  bool hasNextPage();
  void showPageAtIndex(int idx);
  void showCurrentPage();
  void showPrevPage();
  void showNextPage();
  void resized() override;
  bool moveLeft(Component*);
  bool moveRight(Component*);
  bool selectNext(int off = 1);
  bool selectPrevious(int off = 1);
  Component* getSelected();

private:
  void shiftIcons(int);

  StretchableLayoutManager rowLayout;
  StretchableLayoutManager colLayout;
  
  double rowProp;
  double rowSpacerProp;
  double colProp;
  ScopedPointer<ImageComponent> selection;
  int selectindex;
  
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Grid)
};
