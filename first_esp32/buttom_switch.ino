#include "Config.h"            

// אתחול הכפתור
void buttonSetup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);// נגד משיכה פנימי מעלה
}

//אם הכפתור לחוץ 
bool buttonPressed() {
  return digitalRead(BUTTON_PIN) == LOW;
}
