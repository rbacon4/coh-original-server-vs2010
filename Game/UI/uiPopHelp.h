#ifndef UIPOPUP_H
#define UIPOPUP_H

int popHelpWindow();
int popHelpTextWindow();
int popHelpEventHappenedByTag(char *tag);
int popHelpEventHappened(int pop_help_event);
void popHelpTick(void);
void resetPopHelp(void);
void loadPopHelpAttribute(char *afile);
void initializePopHelp(void);


#endif
