#include "Grid.h"
#include "Utils.h"

GridPage::GridPage(int numCols, int numRows) :
  numCols(numCols),
  numRows(numRows),
  gridRows()
{
  for (int i = 0; i < numRows; i++) {
    gridRows.add(new Component());
    addAndMakeVisible(gridRows.getLast());
  }
}

bool GridPage::addItem(Component *item) {
  if (items.size() == numRows * numCols) return false;
  
  // add to flat list
  items.add(item);
  
  // add to view tree in row container
  int row = (items.size() - 1) / numCols;
  gridRows[row]->addAndMakeVisible(item);
  
  return true;
}

bool GridPage::removeItem(Component* item){
  int index = items.indexOf(item);
  if(index!=-1){
    items.removeFirstMatchingValue(item);
    if(index >= numCols) gridRows[1]->removeChildComponent(item);
    else{
      gridRows[0]->removeChildComponent(item);
      /* As one item is missing on our first line, let's try to
      Take one for the next line and move it up */
      if(items.size() >= numCols){
	Component* first = items[2];
	gridRows[0]->addAndMakeVisible(first);
	gridRows[1]->removeChildComponent(first);
      }
    }
    return true;
  }
  return false;
}

bool GridPage::moveRight(Component* item){
  int index = items.indexOf(item);
  int lastindex = numCols*numRows-1;
  //Check whether it's in the current grid or the last item
  if(index == -1 || index == lastindex) return false;
  Component* previous = items[index+1];
  items.swap(index, index+1);
  //Now let's swap the the grid rows
  int item_row = 0;
  int next_row = 0;
  if(index == numCols-1){ 
    item_row = 0; 
    next_row = 1;
    gridRows[item_row]->removeChildComponent(item);
    gridRows[next_row]->removeChildComponent(previous);
    gridRows[item_row]->addAndMakeVisible(previous);
    gridRows[next_row]->addAndMakeVisible(item);
  }
  return true;
}


bool GridPage::moveLeft(Component* item){
  int index = items.indexOf(item);
  //Check whether it's in the current grid or the first item
  if(index == -1 || index == 0) return false;
  Component* previous = items[index-1];
  items.swap(index, index-1);
  //Now let's swap the the grid rows
  int next_row = 0;
  if(index == numCols){ 
    int item_row = 1; 
    gridRows[item_row]->removeChildComponent(item);
    gridRows[next_row]->removeChildComponent(previous);
    gridRows[item_row]->addAndMakeVisible(previous);
    gridRows[next_row]->addAndMakeVisible(item);
  }
  return true;
}

void GridPage::switchLastToFirst(GridPage* next){
  int lastindex = items.size()-1;
  Component* curitem  = items[lastindex];
  Component* first = next->items[0];
  gridRows[numRows-1]->removeChildComponent(curitem);
  next->gridRows[0]->removeChildComponent(first);
  gridRows[numRows-1]->addAndMakeVisible(first);
  next->gridRows[0]->addAndMakeVisible(curitem);
  
  items.set(lastindex, first);
  (next->items).set(0, curitem);
}

void GridPage::resized() {}

Grid::Grid(int numCols, int numRows) :
  numCols(numCols),
  numRows(numRows),
  selection(new ImageComponent()),
  selectindex(0)
{
  page = new GridPage(numCols, numRows);
  pages.add(page);

  /* We need to put this line here to have the selection
   * under the icons and not over */
  addAndMakeVisible(selection);
  selection->setVisible(false);

  addAndMakeVisible(page);

  //Selection square
  Image image = createImageFromFile(assetFile("llselection.png"));
  selection->setImage(image, RectanglePlacement::stretchToFit);
  selection->setName("Select");

  // mildly convoluted way of finding proportion of available height to give each row,
  // accounting for spacer rows which are relatively heighted based on this measure.
  // First measures row proportion without spacers.
  rowProp = (1.0f/numRows);
  // Then find an appropriate relative spacer proportion.
  rowSpacerProp = rowProp / 8.;
  // Reduce ideal row proportion by room taken up by spacers.
  double rowAmountWithoutSpacers = 1.0 - (rowSpacerProp * (numRows - 1));
  rowProp = (rowAmountWithoutSpacers / numRows);
  
  // columns lack spacers and are trivial to proportion.
  colProp = (1.0f/numCols);
  
  int layoutIdx = 0;
  for (int i = 0; i < numRows; i++) {
    rowLayout.setItemLayout(layoutIdx, -rowProp/4, -rowProp, -rowProp);
    layoutIdx++;
    // set size preference for spacer if there are rows left in the loop
    if ((i+1) < numRows) {
      rowLayout.setItemLayout(layoutIdx, -rowSpacerProp, -rowSpacerProp, -rowSpacerProp);
      layoutIdx++;
    }
  }
  for (int i = 0; i < numCols; i++) {
    colLayout.setItemLayout(i, -colProp/4, -colProp, -colProp);
  }
}

void Grid::createPage() {
  pages.add(new GridPage(numCols, numRows));
}

void Grid::addItem(Component *item) {
  items.add(item);
  bool wasAdded = pages.getLast()->addItem(item);
  if (!wasAdded) {
    createPage();
    pages.getLast()->addItem(item);
  }
}

void Grid::removeItem(Component* item){
  //items.remove(item);
  items.removeFirstMatchingValue(item);
  bool wasdeleted = page->removeItem(item);
  if(!wasdeleted) return;
  //Getting next item
  int index = pages.indexOf(page);
  //items.add(newicon);
  shiftIcons(index);
  GridPage* last = pages.getLast();
  /* If after shifting the last page doesn't have any icon anymore
   * we must delete the last page */
  if(last->items.size() == 0){
    /* If the current page is the last, we msut switch to the previous one,
     * else, we get a SEGFAULT */
    if(page == last) showPageAtIndex(index-1);
    pages.removeLast();
  }
  
  showCurrentPage();
}

/* Function used to shift the icons from one page to another
 * After deleting an icon 
 */
void Grid::shiftIcons(int index){
  int nextpage = index + 1;
  bool hasnextpage = nextpage < pages.size() && 
		     pages[nextpage]->items.size() != 0;
  if(!hasnextpage) return;
  
  Component* newicon = pages[nextpage]->items[0];
  pages[nextpage]->removeItem(newicon);
  pages[index]->addItem(newicon);
  
  shiftIcons(nextpage);
}

/**
 * Returns whether a new page has been displayed or not
 */
bool Grid::selectNext(int off){
  selection->setVisible(true);
  int total = page->items.size();

  if(selectindex + off >= total  && !hasNextPage())
    return false;

  selectindex += off;
  if(selectindex >= total){
    selectindex %= total;
    showNextPage();
    return true;
  } else {
    resized();
  }
  return false;
}

bool Grid::selectPrevious(int off){
  selection->setVisible(true);
  if(selectindex - off < 0 && !hasPrevPage())
    return false;

  int total = numCols*numRows;
  selectindex -= off;

  if(selectindex < 0){
    selectindex = (selectindex + total)%total;
    showPrevPage();
    return true;
  } else {
    resized();
  }
  return false;
}

Component* Grid::getSelected(){
  return page->items[selectindex];
}

void Grid::resized() {
  const auto& bounds = getLocalBounds();

  // create row components list for use in stretch layout
  // include room for rows and spacers
  int numRowComps = (2*page->gridRows.size()) - 1;
  Component* rowComps[numRowComps];
  for (int i = 0, j = 0; i < numRows; i++) {
    rowComps[j++] = page->gridRows[i];
    // add spacer null component
    if ((i+1) < numRows) {
      rowComps[j++] = nullptr;
    }
  }

  // fill item row lists
  // create row components list for use in stretch layout
  Component* itemRows[numRows][numCols];
  for (int i = 0; i < numRows; i++) {
    for (int j = 0; j < numCols; j++) {
      auto item = page->items[j + (i*numCols)];
      itemRows[i][j] = item;
    }
  }
  
  // get row height
  double rowHeight = bounds.getHeight() * rowProp;
  double colWidth  = bounds.getWidth() * colProp;

  // size from largest to smallest, stretchable
  // set page size to grid size
  page->setBounds(bounds);
  
  //Draw selection rectangle at first
  int x = 6+(selectindex%numCols)*((int) colWidth);
  int y = (selectindex/numCols)*(rowHeight);
  selection->setBounds(bounds.getX()+x, bounds.getY()+y, colWidth - 10, selectindex/numCols == 0 ? rowHeight : rowHeight + 20);
  
  // lay out components, size the rows first
  rowLayout.layOutComponents(rowComps, numRowComps, bounds.getX(), bounds.getY(),
                             bounds.getWidth(), bounds.getHeight(),
                             true, true);
  
  // size items within rows, creating columns
  for (int i = 0; i < numRows; i++) {
    auto& itemsRow = itemRows[i];
    // columns are laid out within rows, using the elements of the row
    colLayout.layOutComponents(itemsRow, numCols, bounds.getX(), bounds.getY(),
                               bounds.getWidth(), rowHeight,
                               false, true);
  }

}

bool Grid::hasPrevPage() {
  return page != pages.getFirst();
}
bool Grid::hasNextPage() {
  return page != pages.getLast();
}

void Grid::showPageAtIndex(int idx) {
  removeChildComponent(page);
  page->setEnabled(false);
  page->setVisible(false);
  
  page = pages[idx];
  
  addAndMakeVisible(page);
  page->setVisible(true);
  page->setEnabled(true);

  resized();
}

void Grid::showCurrentPage(){  
  int i = pages.indexOf(page);
  showPageAtIndex(i);
}

void Grid::showPrevPage() {
  if (hasPrevPage()) {
    int i = pages.indexOf(page);
    showPageAtIndex(i-1);
  }
}

void Grid::showNextPage() {
  if (hasNextPage()) {
    int i = pages.indexOf(page);
    showPageAtIndex(i+1);
  }
}

/* Method exchanging the place of selected icon (item) with 
 * previous one
 */
bool Grid::moveLeft(Component* item){
  if(!page) return false;
  bool moved = page->moveLeft(item);
  /* If item is the first icon, we have to do the job from here */
  if(!moved && hasPrevPage())
    pages[pages.indexOf(page)-1]->switchLastToFirst(page);
  else if(!moved) return false;
  
  showCurrentPage();
  return true;
}

/* Method exchanging the place of selected icon (item) with 
 * next one
 */
bool Grid::moveRight(Component* item){
  if(!page) return false;
  bool moved = page->moveRight(item);
  /* If item is the last icon of the page, we have to do the job from here */
  if(!moved && hasNextPage())
    page->switchLastToFirst(pages[pages.indexOf(page)+1]);
  else if(!moved) return false;
  
  showCurrentPage();
  return true;
}
