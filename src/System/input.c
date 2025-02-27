/****************************/
/*     INPUT.C			    */
/* (c)1997 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"


/**********************/
/*     PROTOTYPES     */
/**********************/

SDL_GameController	*gSDLController = NULL;
SDL_JoystickID		gSDLJoystickInstanceID = -1;		// ID of the joystick bound to gSDLController

Byte				gRawKeyboardState[SDL_NUM_SCANCODES];
bool				gAnyNewKeysPressed = false;
char				gTextInput[SDL_TEXTINPUTEVENT_TEXT_SIZE];

Byte				gNeedStates[NUM_CONTROL_NEEDS];



/****************************/
/*    CONSTANTS             */
/****************************/

enum
{
	KEYSTATE_OFF		= 0b00,
	KEYSTATE_UP			= 0b01,

	KEYSTATE_PRESSED	= 0b10,
	KEYSTATE_HELD		= 0b11,

	KEYSTATE_ACTIVE_BIT	= 0b10,
};

#define JOYSTICK_DEAD_ZONE .1f
#define JOYSTICK_DEAD_ZONE_SQUARED (JOYSTICK_DEAD_ZONE*JOYSTICK_DEAD_ZONE)

#define JOYSTICK_FAKEDIGITAL_DEAD_ZONE .66f

#if __APPLE__
#define DEFAULTKB1_JUMP		SDL_SCANCODE_LGUI
#define DEFAULTKB2_JUMP		SDL_SCANCODE_RGUI
#define DEFAULTKB1_PICKUP	SDL_SCANCODE_LALT
#define DEFAULTKB2_PICKUP	SDL_SCANCODE_RALT
#else
#define DEFAULTKB1_JUMP		SDL_SCANCODE_LALT
#define DEFAULTKB2_JUMP		SDL_SCANCODE_RALT
#define DEFAULTKB1_PICKUP	SDL_SCANCODE_LCTRL
#define DEFAULTKB2_PICKUP	SDL_SCANCODE_RCTRL
#endif

const KeyBinding kDefaultKeyBindings[NUM_CONTROL_NEEDS] =
{
//Need------------------    Keys--------------------------------------------- MouseBtn-------  MWheel-  GamepadButtons----------------------------------------------------------  GamepadAxis--------------  GamepadAxisSign
[kNeed_Forward			] = {{SDL_SCANCODE_UP,		0,						}, 0,					0,	{SDL_CONTROLLER_BUTTON_DPAD_UP,		SDL_CONTROLLER_BUTTON_INVALID,		}, SDL_CONTROLLER_AXIS_LEFTY,		-1,		},
[kNeed_Backward			] = {{SDL_SCANCODE_DOWN,	0,						}, 0,					0,	{SDL_CONTROLLER_BUTTON_DPAD_DOWN,	SDL_CONTROLLER_BUTTON_INVALID,		}, SDL_CONTROLLER_AXIS_LEFTY,		+1,		},
[kNeed_TurnLeft			] = {{SDL_SCANCODE_LEFT,	0,						}, 0,					0,	{SDL_CONTROLLER_BUTTON_DPAD_LEFT,	SDL_CONTROLLER_BUTTON_INVALID,		}, SDL_CONTROLLER_AXIS_LEFTX,		-1,		},
[kNeed_TurnRight		] = {{SDL_SCANCODE_RIGHT,	0,						}, 0,					0,	{SDL_CONTROLLER_BUTTON_DPAD_RIGHT,	SDL_CONTROLLER_BUTTON_INVALID,		}, SDL_CONTROLLER_AXIS_LEFTX,		+1,		},
[kNeed_JetUp			] = {{SDL_SCANCODE_A,		0,						}, 0,					0,	{SDL_CONTROLLER_BUTTON_INVALID,		SDL_CONTROLLER_BUTTON_INVALID,		}, SDL_CONTROLLER_AXIS_TRIGGERLEFT,	+1,		},
[kNeed_JetDown			] = {{SDL_SCANCODE_Z,		0,						}, 0,					0,	{SDL_CONTROLLER_BUTTON_INVALID,		SDL_CONTROLLER_BUTTON_INVALID,		}, SDL_CONTROLLER_AXIS_TRIGGERRIGHT,+1,		},
//[kNeed_PrevWeapon		] = {{0,					0,						}, 0,					+1,	{SDL_CONTROLLER_BUTTON_LEFTSHOULDER,SDL_CONTROLLER_BUTTON_INVALID,		}, SDL_CONTROLLER_AXIS_INVALID,		0,		},
[kNeed_AttackMode		] = {{SDL_SCANCODE_LSHIFT,	SDL_SCANCODE_RSHIFT,	}, 0,					-1,	{SDL_CONTROLLER_BUTTON_Y, 			SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,}, SDL_CONTROLLER_AXIS_INVALID,		0,		},
[kNeed_Attack			] = {{SDL_SCANCODE_SPACE,	0,						}, SDL_BUTTON_LEFT,		0,	{SDL_CONTROLLER_BUTTON_X,			SDL_CONTROLLER_BUTTON_INVALID,		}, SDL_CONTROLLER_AXIS_INVALID,		0,		},
[kNeed_PickUp			] = {{DEFAULTKB1_PICKUP,	DEFAULTKB2_PICKUP,		}, SDL_BUTTON_MIDDLE,	0,	{SDL_CONTROLLER_BUTTON_B,			SDL_CONTROLLER_BUTTON_INVALID,		}, SDL_CONTROLLER_AXIS_INVALID,		0,		},
[kNeed_Jump				] = {{DEFAULTKB1_JUMP,		DEFAULTKB2_JUMP,		}, SDL_BUTTON_RIGHT,	0,	{SDL_CONTROLLER_BUTTON_A,			SDL_CONTROLLER_BUTTON_INVALID,		}, SDL_CONTROLLER_AXIS_INVALID,		0,		},
[kNeed_CameraMode		] = {{SDL_SCANCODE_TAB,		0,						}, 0,					0,	{SDL_CONTROLLER_BUTTON_RIGHTSTICK,	SDL_CONTROLLER_BUTTON_INVALID,		}, SDL_CONTROLLER_AXIS_INVALID,		0,		},
[kNeed_CameraLeft		] = {{SDL_SCANCODE_COMMA,	0,						}, 0,					0,	{SDL_CONTROLLER_BUTTON_INVALID,		SDL_CONTROLLER_BUTTON_INVALID,		}, SDL_CONTROLLER_AXIS_RIGHTX,		-1,		},
[kNeed_CameraRight		] = {{SDL_SCANCODE_PERIOD,	0,						}, 0,					0,	{SDL_CONTROLLER_BUTTON_INVALID,		SDL_CONTROLLER_BUTTON_INVALID,		}, SDL_CONTROLLER_AXIS_RIGHTX,		+1,		},
[kNeed_ZoomIn			] = {{SDL_SCANCODE_2,		0,						}, 0,					0,	{SDL_CONTROLLER_BUTTON_INVALID,		SDL_CONTROLLER_BUTTON_INVALID,		}, SDL_CONTROLLER_AXIS_RIGHTY,		-1,		},
[kNeed_ZoomOut			] = {{SDL_SCANCODE_1,		0,						}, 0,					0,	{SDL_CONTROLLER_BUTTON_INVALID,		SDL_CONTROLLER_BUTTON_INVALID,		}, SDL_CONTROLLER_AXIS_RIGHTY,		+1,		},
[kNeed_ToggleGPS		] = {{SDL_SCANCODE_G,		0,						}, 0,					0,	{SDL_CONTROLLER_BUTTON_BACK,		SDL_CONTROLLER_BUTTON_INVALID,		}, SDL_CONTROLLER_AXIS_INVALID,		0,		},
[kNeed_UIUp				] = {{SDL_SCANCODE_UP,		0,						}, 0,					0,	{SDL_CONTROLLER_BUTTON_DPAD_UP,		SDL_CONTROLLER_BUTTON_INVALID,		}, SDL_CONTROLLER_AXIS_LEFTY,		-1,		},
[kNeed_UIDown			] = {{SDL_SCANCODE_DOWN,	0,						}, 0,					0,	{SDL_CONTROLLER_BUTTON_DPAD_DOWN,	SDL_CONTROLLER_BUTTON_INVALID,		}, SDL_CONTROLLER_AXIS_LEFTY,		+1,		},
[kNeed_UILeft			] = {{SDL_SCANCODE_LEFT,	0,						}, 0,					0,	{SDL_CONTROLLER_BUTTON_DPAD_LEFT,	SDL_CONTROLLER_BUTTON_INVALID,		}, SDL_CONTROLLER_AXIS_LEFTX,		-1,		},
[kNeed_UIRight			] = {{SDL_SCANCODE_RIGHT,	0,						}, 0,					0,	{SDL_CONTROLLER_BUTTON_DPAD_RIGHT,	SDL_CONTROLLER_BUTTON_INVALID,		}, SDL_CONTROLLER_AXIS_LEFTX,		+1,		},
[kNeed_UIConfirm		] = {{SDL_SCANCODE_RETURN,	SDL_SCANCODE_SPACE,		}, 0,					0,	{SDL_CONTROLLER_BUTTON_START,		SDL_CONTROLLER_BUTTON_A,			}, SDL_CONTROLLER_AXIS_INVALID,		0,		},
[kNeed_UIBack			] = {{SDL_SCANCODE_ESCAPE,	0,						}, 0,					0,	{SDL_CONTROLLER_BUTTON_BACK,		SDL_CONTROLLER_BUTTON_B,			}, SDL_CONTROLLER_AXIS_INVALID,		0,		},
[kNeed_UIPause			] = {{SDL_SCANCODE_ESCAPE,	0,						}, 0,					0,	{SDL_CONTROLLER_BUTTON_START,		SDL_CONTROLLER_BUTTON_INVALID,		}, SDL_CONTROLLER_AXIS_INVALID,		0,		},
[kNeed_ToggleMusic		] = {{SDL_SCANCODE_F9,		0,						}, 0,					0,	{SDL_CONTROLLER_BUTTON_INVALID,		SDL_CONTROLLER_BUTTON_INVALID,		}, SDL_CONTROLLER_AXIS_INVALID,		0,		},
[kNeed_ToggleAmbient	] = {{SDL_SCANCODE_F10,		0,						}, 0,					0,	{SDL_CONTROLLER_BUTTON_INVALID,		SDL_CONTROLLER_BUTTON_INVALID,		}, SDL_CONTROLLER_AXIS_INVALID,		0,		},
[kNeed_ToggleFullscreen	] = {{SDL_SCANCODE_F11,		0,						}, 0,					0,	{SDL_CONTROLLER_BUTTON_INVALID,		SDL_CONTROLLER_BUTTON_INVALID,		}, SDL_CONTROLLER_AXIS_INVALID,		0,		},
};


/**********************/
/*     VARIABLES      */
/**********************/


/**********************/
/* STATIC FUNCTIONS   */
/**********************/

static inline void UpdateKeyState(Byte* state, bool downNow)
{
	switch (*state)	// look at prev state
	{
		case KEYSTATE_HELD:
		case KEYSTATE_PRESSED:
			*state = downNow ? KEYSTATE_HELD : KEYSTATE_UP;
			break;
		case KEYSTATE_OFF:
		case KEYSTATE_UP:
		default:
			*state = downNow ? KEYSTATE_PRESSED : KEYSTATE_OFF;
			break;
	}
}

/************************* INIT INPUT *********************************/

void InitInput(void)
{
}


void UpdateInput(void)
{

	gTextInput[0] = '\0';


	/**********************/
	/* DO SDL MAINTENANCE */
	/**********************/

//	MouseSmoothing_StartFrame();

	int mouseWheelDelta = 0;

	SDL_PumpEvents();
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_QUIT:
				ExitToShell();			// throws Pomme::QuitRequest
				return;

			case SDL_WINDOWEVENT:
				switch (event.window.event)
				{
					case SDL_WINDOWEVENT_CLOSE:
						ExitToShell();	// throws Pomme::QuitRequest
						return;

					case SDL_WINDOWEVENT_RESIZED:
						QD3D_OnWindowResized(event.window.data1, event.window.data2);
						break;

/*
					case SDL_WINDOWEVENT_FOCUS_LOST:
#if __APPLE__
						// On Mac, always restore system mouse accel if cmd-tabbing away from the game
						RestoreMacMouseAcceleration();
#endif
						break;

					case SDL_WINDOWEVENT_FOCUS_GAINED:
#if __APPLE__
						// On Mac, kill mouse accel when focus is regained only if the game has captured the mouse
						if (SDL_GetRelativeMouseMode())
							KillMacMouseAcceleration();
#endif
						break;
*/
				}
				break;

			case SDL_TEXTINPUT:
				memcpy(gTextInput, event.text.text, sizeof(gTextInput));
				_Static_assert(sizeof(gTextInput) == sizeof(event.text.text), "size mismatch: gTextInput / event.text.text");
				break;

/*
			case SDL_MOUSEMOTION:
				if (!gEatMouse)
				{
					MouseSmoothing_OnMouseMotion(&event.motion);
				}
				break;
*/

			case SDL_JOYDEVICEADDED:	 // event.jdevice.which is the joy's INDEX (not an instance id!)
				TryOpenController(false);
				break;

			case SDL_JOYDEVICEREMOVED:	// event.jdevice.which is the joy's UNIQUE INSTANCE ID (not an index!)
				OnJoystickRemoved(event.jdevice.which);
				break;

			case SDL_MOUSEWHEEL:
				mouseWheelDelta += event.wheel.y;
				mouseWheelDelta += event.wheel.x;
				break;
		}
	}

	int numkeys = 0;
	const UInt8* keystate = SDL_GetKeyboardState(&numkeys);
	uint32_t mouseButtons = SDL_GetMouseState(NULL, NULL);

	gAnyNewKeysPressed = false;

	{
		int minNumKeys = numkeys < SDL_NUM_SCANCODES ? numkeys : SDL_NUM_SCANCODES;

		for (int i = 0; i < minNumKeys; i++)
		{
			UpdateKeyState(&gRawKeyboardState[i], keystate[i]);
			if (gRawKeyboardState[i] == KEYSTATE_PRESSED)
				gAnyNewKeysPressed = true;
		}

		// fill out the rest
		for (int i = minNumKeys; i < SDL_NUM_SCANCODES; i++)
			UpdateKeyState(&gRawKeyboardState[i], false);
	}

	// --------------------------------------------


	for (int i = 0; i < NUM_CONTROL_NEEDS; i++)
	{
		const KeyBinding* kb = &gGamePrefs.keys[i];

		bool downNow = false;

		for (int j = 0; j < KEYBINDING_MAX_KEYS; j++)
			if (kb->key[j] && kb->key[j] < numkeys)
				downNow |= 0 != keystate[kb->key[j]];

		if (kb->mouseButton)
			downNow |= 0 != (mouseButtons & SDL_BUTTON(kb->mouseButton));

		if ((kb->mouseWheelDelta > 0 && mouseWheelDelta > 0) || (kb->mouseWheelDelta < 0 && mouseWheelDelta < 0))
			downNow |= true;

		if (gSDLController)
		{
			for (int j = 0; j < KEYBINDING_MAX_GAMEPAD_BUTTONS; j++)
				if (kb->gamepadButton[j] != SDL_CONTROLLER_BUTTON_INVALID)
					downNow |= 0 != SDL_GameControllerGetButton(gSDLController, kb->gamepadButton[j]);

			if (kb->gamepadAxis != SDL_CONTROLLER_AXIS_INVALID)
			{
				int16_t rawValue = SDL_GameControllerGetAxis(gSDLController, kb->gamepadAxis);
				downNow |= (kb->gamepadAxisSign > 0 && rawValue > (int16_t)(JOYSTICK_FAKEDIGITAL_DEAD_ZONE * 32767.0f))
						|| (kb->gamepadAxisSign < 0 && rawValue < (int16_t)(JOYSTICK_FAKEDIGITAL_DEAD_ZONE * -32768.0f));
			}
		}

		UpdateKeyState(&gNeedStates[i], downNow);
	}


	if (GetNewNeedState(kNeed_ToggleFullscreen))
	{
		gGamePrefs.fullscreen = gGamePrefs.fullscreen ? 0 : 1;
		SetFullscreenMode();
	}
}


/************************ GET SKIP KEY STATE ***************************/

bool UserWantsOut(void)
{
	return GetNewNeedState(kNeed_UIConfirm) || GetNewNeedState(kNeed_UIBack);
}


#pragma mark -


bool GetNewSDLKeyState(unsigned short sdlScanCode)
{
	return gRawKeyboardState[sdlScanCode] == KEYSTATE_PRESSED;
}

bool GetSDLKeyState(unsigned short sdlScanCode)
{
	return gRawKeyboardState[sdlScanCode] == KEYSTATE_PRESSED || gRawKeyboardState[sdlScanCode] == KEYSTATE_HELD;
}

bool AreAnyNewKeysPressed(void)
{
	return gAnyNewKeysPressed;
}

bool GetNeedState(int needID)
{
	GAME_ASSERT(needID < NUM_CONTROL_NEEDS);
	return 0 != (gNeedStates[needID] & KEYSTATE_ACTIVE_BIT);
}

bool GetNewNeedState(int needID)
{
	GAME_ASSERT(needID < NUM_CONTROL_NEEDS);
	return gNeedStates[needID] == KEYSTATE_PRESSED;
}


#pragma mark -

/****************************** SDL JOYSTICK FUNCTIONS ********************************/

SDL_GameController* TryOpenController(bool showMessage)
{
	if (gSDLController)
	{
		printf("Already have a valid controller.\n");
		return gSDLController;
	}

	if (SDL_NumJoysticks() == 0)
	{
		return NULL;
	}

	for (int i = 0; gSDLController == NULL && i < SDL_NumJoysticks(); ++i)
	{
		if (SDL_IsGameController(i))
		{
			gSDLController = SDL_GameControllerOpen(i);
			gSDLJoystickInstanceID = SDL_JoystickGetDeviceInstanceID(i);
		}
	}

	if (!gSDLController)
	{
		printf("Joystick(s) found, but none is suitable as an SDL_GameController.\n");
		if (showMessage)
		{
			char messageBuf[1024];
			snprintf(messageBuf, sizeof(messageBuf),
					 "The game does not support your controller yet (\"%s\").\n\n"
					 "You can play with the keyboard and mouse instead. Sorry!",
					 SDL_JoystickNameForIndex(0));
			SDL_ShowSimpleMessageBox(
					SDL_MESSAGEBOX_WARNING,
					"Controller not supported",
					messageBuf,
					gSDLWindow);
		}
		return NULL;
	}

	printf("Opened joystick %d as controller: %s\n", gSDLJoystickInstanceID, SDL_GameControllerName(gSDLController));

	/*
	gSDLHaptic = SDL_HapticOpenFromJoystick(SDL_GameControllerGetJoystick(gSDLController));
	if (!gSDLHaptic)
		printf("This joystick can't do haptic.\n");
	else
		printf("This joystick can do haptic!\n");
	*/

	return gSDLController;
}

void OnJoystickRemoved(SDL_JoystickID which)
{
	if (NULL == gSDLController)		// don't care, I didn't open any controller
		return;

	if (which != gSDLJoystickInstanceID)	// don't care, this isn't the joystick I'm using
		return;

	printf("Current joystick was removed: %d\n", which);

	// Nuke reference to this controller+joystick
	SDL_GameControllerClose(gSDLController);
	gSDLController = NULL;
	gSDLJoystickInstanceID = -1;

	// Try to open another joystick if any is connected.
	TryOpenController(false);
}

static TQ3Vector2D GetThumbStickVector(bool rightStick)
{
	Sint16 dxRaw = SDL_GameControllerGetAxis(gSDLController, rightStick ? SDL_CONTROLLER_AXIS_RIGHTX : SDL_CONTROLLER_AXIS_LEFTX);
	Sint16 dyRaw = SDL_GameControllerGetAxis(gSDLController, rightStick ? SDL_CONTROLLER_AXIS_RIGHTY : SDL_CONTROLLER_AXIS_LEFTY);

	float dx = dxRaw / 32767.0f;
	float dy = dyRaw / 32767.0f;

	float magnitudeSquared = dx*dx + dy*dy;
	if (magnitudeSquared < JOYSTICK_DEAD_ZONE_SQUARED)
		return (TQ3Vector2D) { 0, 0 };
	else
		return (TQ3Vector2D) { dx, dy };
}
