#pragma once
#ifndef ___GLUT_HPP___
#define ___GLUT_HPP___

/* THANKS TO : http://www.rojtberg.net/413/announcing-glut-cpp/
 * Author: Pavel Rojtberg
 * Web: http://launchpad.net/~rojtberg
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef NO_FREEGLUT
#include <GL/glut.h>
#else
#include <GL/freeglut.h>
#endif

#include <unordered_map>

namespace glut {

inline void Init(int argc, char* argv[]) {
    glutInit(&argc, argv);
}

inline void InitDisplayMode(const unsigned int displayMode) {
    glutInitDisplayMode(displayMode);
}

inline void MainLoop() {
    glutMainLoop();
}

inline void IgnoreKeyRepeat(int ignore)
{
	glutIgnoreKeyRepeat(ignore);
}

inline void SetOption(GLenum eWhat, int value)
{
	glutSetOption(eWhat, value);
}

#ifndef NO_FREEGLUT
inline void LeaveMainLoop() {
    glutLeaveMainLoop();
}
#endif

inline void IdleFunc(void(*callback)(void)) {
    glutIdleFunc(callback);
}

class Window {
    friend class Menu;
private:
    int id;

    static std::unordered_map<int, Window*>& getWins()
    {
        static std::unordered_map<int, Window*> wins;
        return wins;
    }

    static void displayFuncSelect() {
        getWins()[glutGetWindow()]->displayFunc();
    }

    static void reshapeFuncSelect(int width, int height) {
        getWins()[glutGetWindow()]->reshapeFunc(width, height);
    }

    static void keyboardFuncSelect(unsigned char key, int x, int y) {
        getWins()[glutGetWindow()]->keyboardFunc(key, x, y);
    }

	static void keyboardUpFuncSelect(unsigned char key, int x, int y) {
		getWins()[glutGetWindow()]->keyboardUpFunc(key, x, y);
	}

    static void specialFuncSelect(int key, int x, int y) {
        getWins()[glutGetWindow()]->specialFunc(key, x, y);
    }

    static void mouseFuncSelect(int button, int state, int x, int y) {
        getWins()[glutGetWindow()]->mouseFunc(button, state, x, y);
    }

    static void motionFuncSelect(int x, int y) {
        getWins()[glutGetWindow()]->motionFunc(x, y);
    }

    static void passiveMotionFuncSelect(int x, int y) {
        getWins()[glutGetWindow()]->passiveMotionFunc(x, y);
    }

	static void timerFuncSelect(int idx) {
		getWins()[glutGetWindow()]->timerFunc(idx);
	}

#ifndef NO_FREEGLUT
    static void closeFuncSelect() {
        getWins()[glutGetWindow()]->closeFunc();
    }
#endif

public:
    Window(const char* title = "", int width = 300, int height = 300, int x = -1, int y = -1) {
        glutInitWindowSize(width, height);
        glutInitWindowPosition(x, y);
        id = glutCreateWindow(title);

        getWins()[id] = this;

        glutDisplayFunc(Window::displayFuncSelect);
        glutKeyboardFunc(Window::keyboardFuncSelect);
		glutKeyboardUpFunc(Window::keyboardUpFuncSelect);
        glutSpecialFunc(Window::specialFuncSelect);
        glutMouseFunc(Window::mouseFuncSelect);
        glutReshapeFunc(Window::reshapeFuncSelect);
        glutMotionFunc(Window::motionFuncSelect);
        glutPassiveMotionFunc(Window::passiveMotionFuncSelect);
#ifndef NO_FREEGLUT
        glutCloseFunc(Window::closeFuncSelect);
#endif
    }

    void setTitle(const char* title) {
        glutSetWindow(id);
        glutSetWindowTitle(title);
    }

    void reshape(int width, int height) {
        glutSetWindow(id);
        glutReshapeWindow(width, height);
    }

    void show() {
        glutSetWindow(id);
        glutShowWindow();
    }

    void hide() {
        glutSetWindow(id);
        glutHideWindow();
    }

    void iconify() {
        glutSetWindow(id);
        glutIconifyWindow();
    }

    void swapBuffers() {
        glutSetWindow(id);
        glutSwapBuffers();
    }

    void postRedisplay() {
        glutSetWindow(id);
        glutPostRedisplay();
    }

    void setCursor(int cursor) {
        glutSetWindow(id);
        glutSetCursor(cursor);
    }

	void createTimer(int mills, int idx) {
		glutSetWindow(id);
		glutTimerFunc(mills, Window::timerFuncSelect, idx);
	}

#ifndef NO_FREEGLUT
    void fullscreenToggle() {
        glutSetWindow(id);
        glutFullScreenToggle();
    }
#endif

    virtual void displayFunc() = 0;

    virtual void keyboardFunc(unsigned char key, int x, int y) {
        // disable events if not overridden
        glutSetWindow(id);
        glutKeyboardFunc(NULL);
    }

	virtual void keyboardUpFunc(unsigned char key, int x, int y) {
        // disable events if not overridden
        glutSetWindow(id);
        glutKeyboardUpFunc(NULL);
    }

    virtual void specialFunc(int key, int x, int y) {
        // disable events if not overridden
        glutSetWindow(id);
        glutSpecialFunc(NULL);
    }

    virtual void mouseFunc(int button, int state, int x, int y) {
        // disable events if not overridden
        glutSetWindow(id);
        glutMouseFunc(NULL);
    }

    virtual void reshapeFunc(int width, int height) {
        // disable events if not overridden
        glutSetWindow(id);
        glutReshapeFunc(NULL);
    }

    virtual void motionFunc(int x, int y) {
        // disable events if not overridden
        glutSetWindow(id);
        glutMotionFunc(NULL);
    }

    virtual void passiveMotionFunc(int x, int y) {
        // disable events if not overridden
        glutSetWindow(id);
        glutPassiveMotionFunc(NULL);
    }

	virtual void timerFunc(int idx) {
		glutSetWindow(id);
		//
	}

#ifndef NO_FREEGLUT
    virtual void closeFunc() {
        // disable events if not overridden
        glutSetWindow(id);
        glutCloseFunc(NULL);
    }
#endif

    ~Window() {
		/* 20150803 glutDestroyWindow(id);をコメントアウト
		   glut::SetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
		   するとこの部分で実行時エラー。おそらくシステム側で破棄されたウィンドウを破棄することになる。
		   破棄済みか調べる関数が不明。おそらく放置してもシステム側で勝手に破棄してくれるので、
		   とりあえずコメントアウト。
		*/
		//glutDestroyWindow(id);
    }
};

class Menu {
private:
    int id;

    static std::unordered_map<int, Menu*>& getMenus()
    {
        static std::unordered_map<int, Menu*> menus;
        return menus;
    }

    static void menuFunSelect(int value) {
        getMenus()[glutGetMenu()]->selected(value);
    }

public:
    Menu(Window& window) {
        glutSetWindow(window.id);
        id = glutCreateMenu(Menu::menuFunSelect);
        getMenus()[id] = this;
    }

    void addEntry(const char* label, int value) {
        glutSetMenu(id);
        glutAddMenuEntry(label, value);
    }

    void attach(int button) {
        glutSetMenu(id);
        glutAttachMenu(button);
    }

    virtual void selected(int value) = 0;
};

}

#endif
