#include "state.h"

static void _tick(State* state);
static void _draw(State* state);

static void
_tick(State* state)
{
	SDL_Event event;
	SDL_MouseWheelEvent* mouseWheelEvent;

	state->input.mouse.wheelDeltaY = 0;

	while(SDL_PollEvent(&event))
	{
    	ImGui_ImplSDL3_ProcessEvent(&event);
		
		switch (event.type)
		{
			case SDL_EVENT_QUIT:
				state->isRunning = false;
				break;
			case SDL_EVENT_MOUSE_WHEEL:
				mouseWheelEvent = &event.wheel;
            	state->input.mouse.wheelDeltaY = mouseWheelEvent->y;
				break;
			default:
				break;
		}
	}

	SDL_GetWindowSize(state->window, &state->settings.windowW, &state->settings.windowH);

	input_tick(&state->input);
	editor_tick(&state->editor);
	preview_tick(&state->preview);
	tool_tick(&state->tool);
	snapshots_tick(&state->snapshots);
	dialog_tick(&state->dialog);
	imgui_tick(&state->imgui);

	if (input_release(&state->input, INPUT_SAVE))
	{
		// Open dialog if path empty, otherwise save in-place
		if (state->anm2.path.empty())
			dialog_anm2_save(&state->dialog);
		else 
			anm2_serialize(&state->anm2, state->anm2.path);
	}

	if (input_release(&state->input, INPUT_PLAY))
	{
		state->preview.isPlaying = !state->preview.isPlaying;
		state->preview.isRecording = false;
	}
}

static void 
_draw(State* state)
{
	editor_draw(&state->editor);
	preview_draw(&state->preview);
	imgui_draw();

	SDL_GL_SwapWindow(state->window);
}

void
init(State* state)
{
	settings_load(&state->settings);
	
	std::cout << STRING_INFO_INIT << std::endl;

	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		std::cout << STRING_ERROR_SDL_INIT << SDL_GetError() << std::endl;
		quit(state);
	}
	
	SDL_CreateWindowAndRenderer
	(
		STRING_WINDOW_TITLE, 
		state->settings.windowW, 
		state->settings.windowH, 
		WINDOW_FLAGS, 
		&state->window, 
		&state->renderer
	);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
   	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
   	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	state->glContext = SDL_GL_CreateContext(state->window);
	
	if (!state->glContext)
	{
		std::cout << STRING_ERROR_GL_CONTEXT_INIT << SDL_GetError() << std::endl;
		quit(state);
	}

	std::cout << STRING_INFO_SDL_INIT << "(" << STRING_INFO_OPENGL << glGetString(GL_VERSION) << ")" << std::endl;
	
	glewInit();
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  
	glDisable(GL_DEPTH_TEST);
	glLineWidth(LINE_WIDTH);

	std::cout << STRING_INFO_GLEW_INIT << std::endl;

	resources_init(&state->resources);
	dialog_init(&state->dialog, &state->anm2, &state->reference, &state->resources, state->window);
	tool_init(&state->tool, &state->input);
	snapshots_init(&state->snapshots, &state->anm2, &state->reference, &state->time, &state->input);
	preview_init(&state->preview, &state->anm2, &state->reference, &state->time, &state->resources, &state->settings);
	editor_init(&state->editor, &state->anm2, &state->reference, &state->resources, &state->settings);
	
	imgui_init
	(
		&state->imgui,
		&state->dialog,
		&state->resources,
		&state->input,
		&state->anm2,
		&state->reference,
		&state->time,
		&state->editor,
		&state->preview,
		&state->settings,
		&state->tool,
		&state->snapshots,
		state->window,
		&state->glContext
	);

	if (state->isArgument)
	{
		anm2_deserialize(&state->anm2, &state->resources, state->argument);
		window_title_from_path_set(state->window, state->argument);
	}
	else
		anm2_new(&state->anm2);
}

void
loop(State* state)
{
	state->tick = SDL_GetTicks();
	
	while (state->tick > state->lastTick + TICK_DELAY)
	{
		state->tick = SDL_GetTicks();
		
		if (state->tick - state->lastTick < TICK_DELAY)
        	SDL_Delay(TICK_DELAY - (state->tick - state->lastTick));

		_tick(state);

		state->lastTick = state->tick;
	}

	_draw(state);
}

void
quit(State* state)
{
	imgui_free();
	settings_save(&state->settings);
	preview_free(&state->preview);
	editor_free(&state->editor);
	resources_free(&state->resources);

	SDL_GL_DestroyContext(state->glContext);
	SDL_Quit();

	std::cout << STRING_INFO_QUIT << std::endl;
}

