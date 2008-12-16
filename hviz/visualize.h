#ifndef VISUALIZE_H
#define VISUALIZE_H

void viz_init();
void on3DDrag(int x, int y, int z);
void on3DRotate(int rx0, int ry0, int rz0);
void on2DDrag(int x, int y);
void onMotion(int x, int y);
void onClick(int button, int state, int x, int y);
void onUpdate();
void onUpdateInfo();
void onReshape(int width, int height);
void onTimer(int value);
void onKeyboard(unsigned char key, int x, int y);
void onKeyboardSpecial(int key, int x, int y);


#endif

