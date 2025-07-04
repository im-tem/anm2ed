#include "imgui.h"

static void _imgui_tooltip(const std::string& tooltip);
static void _imgui_timeline_item_frames(Imgui* self, Anm2Reference reference, s32* index);
static void _imgui_timeline_item(Imgui* self, Anm2Reference reference, s32* index);
static void _imgui_timeline(Imgui* self);
static void _imgui_animations(Imgui* self);
static void _imgui_events(Imgui* self);
static void _imgui_spritesheets(Imgui* self);
static void _imgui_frame_properties(Imgui* self);
static void _imgui_spritesheet_editor(Imgui* self);
static void _imgui_animation_preview(Imgui* self);
static void _imgui_taskbar(Imgui* self);
static void _imgui_undoable(Imgui* self);
static void _imgui_undo_stack_push(Imgui* self);
static void _imgui_spritesheet_editor_set(Imgui* self, s32 spritesheetID);

// Push undo stack using imgui's members
static void _imgui_undo_stack_push(Imgui* self)
{
    Snapshot snapshot = {*self->anm2, *self->reference, *self->time};
    snapshots_undo_stack_push(self->snapshots, &snapshot);
}

// Tooltip for the last hovered widget
static void _imgui_tooltip(const std::string& tooltip)
{
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
        ImGui::SetTooltip(tooltip.c_str());
}

// If the last widget is activated, pushes the undo stack
static void _imgui_undoable(Imgui* self)
{
    if (ImGui::IsItemActivated())
		_imgui_undo_stack_push(self);
}

// Will push a new spritesheetID to the editor
static void _imgui_spritesheet_editor_set(Imgui* self, s32 spritesheetID)
{
	// Make sure the spritesheet exists!
	if (self->anm2->spritesheets.contains(spritesheetID))
		self->editor->spritesheetID = spritesheetID;
}

// Drawing the frames of an Anm2Item
static void
_imgui_timeline_item_frames(Imgui* self, Anm2Reference reference, s32* index) 
{
	ImVec2 frameStartPos;
	ImVec2 framePos;
	ImVec2 frameFinishPos;
	ImVec2 cursorPos = ImGui::GetCursorPos();
	ImVec4 frameColor;
	ImVec4 hoveredColor;
	ImVec4 activeColor;
	
	Anm2Animation* animation = anm2_animation_from_reference(self->anm2, &reference);
	Anm2Item* item = anm2_item_from_reference(self->anm2, &reference);

	switch (reference.itemType)
	{
		case ANM2_ROOT:
			frameColor = IMGUI_TIMELINE_ROOT_FRAME_COLOR;
			hoveredColor = IMGUI_TIMELINE_ROOT_HIGHLIGHT_COLOR;
			activeColor = IMGUI_TIMELINE_ROOT_ACTIVE_COLOR;
			break;
		case ANM2_LAYER:
			frameColor = IMGUI_TIMELINE_LAYER_FRAME_COLOR;
			hoveredColor = IMGUI_TIMELINE_LAYER_HIGHLIGHT_COLOR;
			activeColor = IMGUI_TIMELINE_LAYER_ACTIVE_COLOR;
			break;
		case ANM2_NULL:
			frameColor = IMGUI_TIMELINE_NULL_FRAME_COLOR;
			hoveredColor = IMGUI_TIMELINE_NULL_HIGHLIGHT_COLOR;
			activeColor = IMGUI_TIMELINE_NULL_ACTIVE_COLOR;
			break;
		case ANM2_TRIGGERS:
			frameColor = IMGUI_TIMELINE_TRIGGERS_FRAME_COLOR;
			hoveredColor = IMGUI_TIMELINE_TRIGGERS_HIGHLIGHT_COLOR;
			activeColor = IMGUI_TIMELINE_TRIGGERS_ACTIVE_COLOR;
			break;
		default:
			break;
	}

	ImGui::PushID(*index);

	// Draw only if the animation has a length above 0
	if (animation->frameNum > 0)
	{
		ImVec2 frameListSize = {IMGUI_TIMELINE_FRAME_SIZE.x * animation->frameNum, IMGUI_TIMELINE_ELEMENTS_TIMELINE_SIZE.y};
		ImVec2 mousePosRelative;

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

		ImGui::BeginChild(STRING_IMGUI_TIMELINE_FRAMES, frameListSize, true);

		// Will deselect frame if hovering and click; but, if it's later clicked, this won't have any effect
		if (ImGui::IsWindowHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
			anm2_reference_frame_clear(self->reference);

		vec2 mousePos = IMVEC2_TO_VEC2(ImGui::GetMousePos());
		vec2 windowPos = IMVEC2_TO_VEC2(ImGui::GetWindowPos());

		f32 scrollX = ImGui::GetScrollX();
		f32 mousePosRelativeX = mousePos.x - windowPos.x - scrollX;

		frameStartPos = ImGui::GetCursorPos();

		// Draw the frame background
		for (s32 i = 0; i < animation->frameNum; i++)
		{
			ImGui::PushID(i);

			ImVec2 frameTexturePos = ImGui::GetCursorScreenPos();

			if (i % IMGUI_TIMELINE_FRAME_INDICES_MULTIPLE == 0)
			{
				ImVec2 bgMin = frameTexturePos;
				ImVec2 bgMax = ImVec2(frameTexturePos.x + IMGUI_TIMELINE_FRAME_SIZE.x, frameTexturePos.y + IMGUI_TIMELINE_FRAME_SIZE.y);
				ImU32 bgColor = ImGui::GetColorU32(IMGUI_FRAME_OVERLAY_COLOR); 
				ImGui::GetWindowDrawList()->AddRectFilled(bgMin, bgMax, bgColor);
			}

			ImGui::Image(self->resources->atlas.id, IMGUI_TIMELINE_FRAME_SIZE, IMVEC2_ATLAS_UV_GET(TEXTURE_FRAME_ALT));

			ImGui::SameLine();
			ImGui::PopID();
		}

		// Draw each frame
		for (auto [i, frame] : std::views::enumerate(item->frames))
		{
			reference.frameIndex = i;

			TextureType textureType;
			f32 frameWidth = IMGUI_TIMELINE_FRAME_SIZE.x * frame.delay;
			ImVec2 frameSize = ImVec2(frameWidth, IMGUI_TIMELINE_FRAME_SIZE.y);

			if (reference.itemType == ANM2_TRIGGERS)
			{
				framePos.x = frameStartPos.x + (IMGUI_TIMELINE_FRAME_SIZE.x * frame.atFrame);
				textureType = TEXTURE_TRIGGER;
			}
			else
				textureType = frame.isInterpolated ? TEXTURE_CIRCLE : TEXTURE_SQUARE;

			ImGui::SetCursorPos(framePos);
			
			ImVec4 buttonColor = *self->reference == reference ? activeColor : frameColor;

			ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoveredColor);			
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeColor);			
			ImGui::PushStyleColor(ImGuiCol_Border, IMGUI_FRAME_BORDER_COLOR);			
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, IMGUI_FRAME_BORDER);
			
			ImGui::PushID(i);

			if (ImGui::Button(STRING_IMGUI_TIMELINE_FRAME_LABEL, frameSize))
			{
				s32 frameTime = (s32)(mousePosRelativeX / IMGUI_TIMELINE_FRAME_SIZE.x);

				*self->reference = reference;

				*self->time = frameTime;

				// Set the preview's spritesheet ID
				if (self->reference->itemType == ANM2_LAYER)
					_imgui_spritesheet_editor_set(self, self->anm2->layers[self->reference->itemID].spritesheetID);
			}

			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
			{
				*self->reference = reference;

				ImGui::SetDragDropPayload(STRING_IMGUI_TIMELINE_FRAME_DRAG_DROP, &reference, sizeof(Anm2Reference));
				ImGui::Button(STRING_IMGUI_TIMELINE_FRAME_LABEL, frameSize);
				ImGui::SetCursorPos(ImVec2(1.0f, (IMGUI_TIMELINE_FRAME_SIZE.y / 2) - (TEXTURE_SIZE_SMALL.y / 2)));
				ImGui::Image(self->resources->atlas.id, VEC2_TO_IMVEC2(ATLAS_SIZES[textureType]), IMVEC2_ATLAS_UV_GET(textureType));
				ImGui::EndDragDropSource();
			}

			if (self->reference->itemID == reference.itemID)
			{
				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(STRING_IMGUI_TIMELINE_FRAME_DRAG_DROP))
					{
						Anm2Reference checkReference = *(Anm2Reference*)payload->Data;
						if (checkReference != reference) 
						{
							self->isSwap = true;
							self->swapReference = reference;
							
							_imgui_undo_stack_push(self);
						}
					}
					ImGui::EndDragDropTarget();
				}
			}

			ImGui::PopStyleVar();
			ImGui::PopStyleColor(4);

			ImGui::SetCursorPos(ImVec2(framePos.x + 1.0f, (framePos.y + (IMGUI_TIMELINE_FRAME_SIZE.y / 2)) - TEXTURE_SIZE_SMALL.y / 2));

			ImGui::Image(self->resources->atlas.id, VEC2_TO_IMVEC2(ATLAS_SIZES[textureType]), IMVEC2_ATLAS_UV_GET(textureType));

			ImGui::PopID();

			if (reference.itemType != ANM2_TRIGGERS) 
				framePos.x += frameWidth;
		}

		ImGui::EndChild();
		ImGui::PopStyleVar(2);

		ImGui::SetCursorPosX(cursorPos.x);
		ImGui::SetCursorPosY(cursorPos.y + IMGUI_TIMELINE_FRAME_SIZE.y);
	}
	else
		ImGui::Dummy(IMGUI_DUMMY_SIZE);

	(*index)++;

	ImGui::PopID();
}

// Displays each item of the timeline of a selected animation
static void
_imgui_timeline_item(Imgui* self, Anm2Reference reference, s32* index) 
{
	static s32 textEntryItemIndex = -1;
	static s32 textEntrySpritesheetIndex = -1;

	TextureType textureType = TEXTURE_ERROR;
	s32* spritesheetID = NULL;
	bool* isShowRect = NULL;
	Anm2Null* null = NULL;
	Anm2Layer* layer = NULL;
	std::string nameVisible;
	std::string* namePointer = NULL;
	char nameBuffer[ANM2_STRING_MAX];
	memset(nameBuffer, '\0', sizeof(nameBuffer));
	
	bool isChangeable = reference.itemType != ANM2_ROOT && reference.itemType != ANM2_TRIGGERS;
	bool isSelected = self->reference->itemID == reference.itemID && self->reference->itemType == reference.itemType;
	bool isTextEntry = textEntryItemIndex == *index && isChangeable;
	bool isSpritesheetTextEntry = textEntrySpritesheetIndex == *index; 

	f32 cursorPosY = ImGui::GetCursorPosY();
	ImVec4 color;

	Anm2Item* item = anm2_item_from_reference(self->anm2, &reference);

	if (!item)
		return;

	switch (reference.itemType)
	{
		case ANM2_ROOT:
			textureType = TEXTURE_ROOT;
			color = IMGUI_TIMELINE_ROOT_COLOR;
			nameVisible = STRING_IMGUI_TIMELINE_ROOT;
			break;
		case ANM2_LAYER:
			textureType = TEXTURE_LAYER;
			color = IMGUI_TIMELINE_LAYER_COLOR;
			layer = &self->anm2->layers[reference.itemID];
			spritesheetID = &layer->spritesheetID;
			namePointer = &layer->name;
			strncpy(nameBuffer, (*namePointer).c_str(), ANM2_STRING_MAX - 1);
			nameVisible = std::format(STRING_IMGUI_TIMELINE_ITEM_FORMAT, reference.itemID, *namePointer);
			break;
		case ANM2_NULL:
			textureType = TEXTURE_NULL;
			color = IMGUI_TIMELINE_NULL_COLOR;
			null = &self->anm2->nulls[reference.itemID];
			isShowRect = &null->isShowRect;
			namePointer = &null->name;
			strncpy(nameBuffer, (*namePointer).c_str(), ANM2_STRING_MAX - 1);
			nameVisible = std::format(STRING_IMGUI_TIMELINE_ITEM_FORMAT, reference.itemID, *namePointer);
			break;
		case ANM2_TRIGGERS:
			textureType = TEXTURE_TRIGGERS;
			color = IMGUI_TIMELINE_TRIGGERS_COLOR;
			nameVisible = STRING_IMGUI_TIMELINE_TRIGGERS;
			break;
		default:
			break;
	}

	ImGui::PushID(*index);

	ImGui::PushStyleColor(ImGuiCol_ChildBg, color);
	ImGui::BeginChild(nameVisible.c_str(), IMGUI_TIMELINE_ELEMENT_SIZE, true, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar);
	ImGui::PopStyleColor();
	
	ImGui::Image(self->resources->atlas.id, VEC2_TO_IMVEC2(ATLAS_SIZES[textureType]), IMVEC2_ATLAS_UV_GET(textureType));
	
	ImGui::SameLine();

	ImGui::BeginChild(STRING_IMGUI_TIMELINE_ELEMENT_NAME_LABEL, IMGUI_TIMELINE_ELEMENT_NAME_SIZE);

	if (isTextEntry)
	{
		if (ImGui::InputText(STRING_IMGUI_TIMELINE_ANIMATION_LABEL, nameBuffer, ANM2_STRING_MAX, ImGuiInputTextFlags_EnterReturnsTrue))
		{
			*namePointer = nameBuffer;
			textEntryItemIndex = -1;
		}
		_imgui_undoable(self);

		if (!ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
			textEntryItemIndex = -1;
	}
	else
	{
		if (ImGui::Selectable(nameVisible.c_str(), isSelected))
		{
			*self->reference = reference;
			anm2_reference_frame_clear(self->reference);
		}

		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			textEntryItemIndex = *index;
	}

	// Drag and drop items (only layers/nulls)
	if (isChangeable && ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
	{
		*self->reference = reference;
		anm2_reference_frame_clear(self->reference);

		ImGui::PushStyleColor(ImGuiCol_ChildBg, color);
		ImGui::SetDragDropPayload(STRING_IMGUI_TIMELINE_ITEM_DRAG_DROP, &reference, sizeof(Anm2Frame));
		ImGui::PopStyleColor();
		ImGui::Image(self->resources->atlas.id, VEC2_TO_IMVEC2(ATLAS_SIZES[textureType]), IMVEC2_ATLAS_UV_GET(textureType));
		ImGui::SameLine();
		ImGui::Text(nameVisible.c_str());
		ImGui::EndDragDropSource();
	}

	if (self->reference->itemType == reference.itemType && ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(STRING_IMGUI_TIMELINE_ITEM_DRAG_DROP))
		{
			Anm2Reference checkReference = *(Anm2Reference*)payload->Data;
			if (checkReference != reference) 
			{
				self->isSwap = true;
				self->swapReference = reference;
				
				_imgui_undo_stack_push(self);
			}
		}
		ImGui::EndDragDropTarget();
	}

	switch (reference.itemType)
	{
		case ANM2_ROOT:
			_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_ELEMENT_ROOT);
			break;
		case ANM2_LAYER:
			_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_ELEMENT_LAYER);
			break;
		case ANM2_NULL:
			_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_ELEMENT_NULL);
			break;
		case ANM2_TRIGGERS:
			_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_ELEMENT_TRIGGERS);
			break;
		default:
			break;
	}

	ImGui::EndChild();

	// IsVisible
	ImVec2 cursorPos;
	TextureType visibleTextureType = item->isVisible ? TEXTURE_VISIBLE : TEXTURE_INVISIBLE;

	ImGui::SameLine();
		
	cursorPos = ImGui::GetCursorPos();
	ImGui::SetCursorPosX(cursorPos.x + ImGui::GetContentRegionAvail().x - IMGUI_ICON_BUTTON_SIZE.x - ImGui::GetStyle().FramePadding.x * 2);

	if 
	(
		ImGui::ImageButton
		(
			STRING_IMGUI_TIMELINE_VISIBLE, 
			self->resources->atlas.id, 
			VEC2_TO_IMVEC2(ATLAS_SIZES[visibleTextureType]), 
			IMVEC2_ATLAS_UV_GET(visibleTextureType)
		)
	)
		item->isVisible = !item->isVisible;
	_imgui_undoable(self);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_ELEMENT_VISIBLE);

	ImGui::SetCursorPos(cursorPos);

	// Spritesheet IDs
	if (spritesheetID)
	{
		std::string spritesheetIDName;

		spritesheetIDName = std::format(STRING_IMGUI_TIMELINE_SPRITESHEET_ID_FORMAT, *spritesheetID);

		ImGui::BeginChild(STRING_IMGUI_TIMELINE_ELEMENT_SPRITESHEET_ID_LABEL, IMGUI_TIMELINE_ELEMENT_SPRITESHEET_ID_SIZE);
			
		ImGui::Image(self->resources->atlas.id, VEC2_TO_IMVEC2(ATLAS_SIZES[TEXTURE_SPRITESHEET]), IMVEC2_ATLAS_UV_GET(TEXTURE_SPRITESHEET));
		ImGui::SameLine();

		if (isSpritesheetTextEntry)
		{
			if (ImGui::InputInt(STRING_IMGUI_TIMELINE_ELEMENT_SPRITESHEET_ID_LABEL, spritesheetID, 0, 0, ImGuiInputTextFlags_None))
				textEntrySpritesheetIndex = -1; 
			_imgui_undoable(self);

			if (!ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
				textEntrySpritesheetIndex = -1;
		}
		else
		{
			ImGui::Selectable(spritesheetIDName.c_str());

			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				textEntrySpritesheetIndex = *index;
		}
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_ELEMENT_SPRITESHEET);

		ImGui::EndChild();
	}
	
	// ShowRect
	if (isShowRect)
	{
		TextureType rectTextureType = *isShowRect ? TEXTURE_RECT : TEXTURE_RECT_HIDE;

		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - ((IMGUI_ICON_BUTTON_SIZE.x - ImGui::GetStyle().FramePadding.x * 2) * 4));
		
		if (ImGui::ImageButton(STRING_IMGUI_TIMELINE_RECT, self->resources->atlas.id, VEC2_TO_IMVEC2(ATLAS_SIZES[rectTextureType]), IMVEC2_ATLAS_UV_GET(rectTextureType)))
			*isShowRect = !*isShowRect;
		_imgui_undoable(self);
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_ELEMENT_RECT);
	}
	
	ImGui::EndChild();
	
	(*index)++; 

	ImGui::PopID();

	ImGui::SetCursorPosY(cursorPosY + IMGUI_TIMELINE_ELEMENT_SIZE.y);
}

// Timeline window
static void
_imgui_timeline(Imgui* self)
{
	ImGui::Begin(STRING_IMGUI_TIMELINE);

	Anm2Animation* animation = anm2_animation_from_reference(self->anm2, self->reference);

	if (animation)
	{
		ImVec2 cursorPos;
		ImVec2 mousePos;
		ImVec2 mousePosRelative;
		s32 index = 0;
		ImVec2 frameSize = IMGUI_TIMELINE_FRAME_SIZE;
		ImVec2 pickerPos;
		ImVec2 lineStart;
		ImVec2 lineEnd;
		ImDrawList* drawList;
		static f32 itemScrollX = 0;
		static f32 itemScrollY = 0;
		static bool isPickerDragging;
		ImVec2 frameIndicesSize = {frameSize.x * animation->frameNum, IMGUI_TIMELINE_FRAME_INDICES_SIZE.y};
		const char* buttonText = self->preview->isPlaying ? STRING_IMGUI_TIMELINE_PAUSE : STRING_IMGUI_TIMELINE_PLAY;
		const char* buttonTooltipText = self->preview->isPlaying ? STRING_IMGUI_TOOLTIP_TIMELINE_PAUSE : STRING_IMGUI_TOOLTIP_TIMELINE_PLAY;
		ImVec2 region = ImGui::GetContentRegionAvail();
		ImVec2 windowSize;
		s32& animationID = self->reference->animationID;

		ImVec2 timelineSize = {region.x, region.y - IMGUI_TIMELINE_OFFSET_Y};
		
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::BeginChild(STRING_IMGUI_TIMELINE_CHILD, timelineSize, true);
		windowSize = ImGui::GetWindowSize();

		cursorPos = ImGui::GetCursorPos();

		drawList = ImGui::GetWindowDrawList();
	
		ImGui::SetCursorPos(ImVec2(cursorPos.x + IMGUI_TIMELINE_ELEMENT_SIZE.x, cursorPos.y + IMGUI_TIMELINE_VIEWER_SIZE.y));
		ImGui::BeginChild(STRING_IMGUI_TIMELINE_ELEMENT_FRAMES, {0, 0}, false, ImGuiWindowFlags_HorizontalScrollbar);
		ImGui::PopStyleVar();
		ImGui::PopStyleVar();
		itemScrollX = ImGui::GetScrollX();
		itemScrollY = ImGui::GetScrollY();

		// Root
		_imgui_timeline_item_frames(self, Anm2Reference{animationID, ANM2_ROOT, 0, 0}, &index);

		// Layers (Reversed)
		for (auto it = animation->layerAnimations.rbegin(); it != animation->layerAnimations.rend(); it++)
		{
			s32 id = it->first;
			_imgui_timeline_item_frames(self, Anm2Reference{animationID, ANM2_LAYER, id, 0}, &index);
		}

		// Nulls
		for (auto & [id, null] : animation->nullAnimations)
			_imgui_timeline_item_frames(self, Anm2Reference{animationID, ANM2_NULL, id, 0}, &index);

		// Triggers
		_imgui_timeline_item_frames(self, Anm2Reference{animationID, ANM2_TRIGGERS, 0, 0}, &index);

		// Swap the two selected elements
		if (self->isSwap)
		{
			Anm2Frame* aFrame = anm2_frame_from_reference(self->anm2, self->reference);
			Anm2Frame* bFrame = anm2_frame_from_reference(self->anm2, &self->swapReference);
			Anm2Frame oldFrame = *aFrame;

			// With triggers, just swap the event Id
			if (self->reference->itemType == ANM2_TRIGGERS)
			{
				aFrame->eventID = bFrame->eventID;
				bFrame->eventID = oldFrame.eventID;
			}
			else
			{
				*aFrame = *bFrame;
				*bFrame = oldFrame;
			}

			self->isSwap = false;
			self->reference->frameIndex = self->swapReference.frameIndex;
			self->swapReference = Anm2Reference{};
		}

		ImGui::EndChild();

		ImGui::SetCursorPos(cursorPos);

		ImGui::PushStyleColor(ImGuiCol_ChildBg, IMGUI_TIMELINE_HEADER_COLOR);
		ImGui::BeginChild(STRING_IMGUI_TIMELINE_HEADER, IMGUI_TIMELINE_ELEMENT_SIZE, true);
		ImGui::EndChild();
		ImGui::PopStyleColor();

		// Only draw if animation length isn't 0
		if (animation->frameNum > 0)
		{
			bool isMouseInElementsRegion = false;

			ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();
			ImVec2 clipRectMin = {cursorScreenPos.x + IMGUI_TIMELINE_ELEMENT_SIZE.x, 0};
			ImVec2 clipRectMax = {cursorScreenPos.x + timelineSize.x + IMGUI_TIMELINE_FRAME_SIZE.x, cursorScreenPos.y + timelineSize.y};
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

			ImGui::SameLine();
			
			ImGui::PushClipRect(clipRectMin, clipRectMax, true);

			ImGui::BeginChild(STRING_IMGUI_TIMELINE_FRAME_INDICES, {0, IMGUI_TIMELINE_FRAME_SIZE.y});
			ImGui::SetScrollX(itemScrollX);
			
			ImVec2 itemsRectMin = ImGui::GetWindowPos();
			ImVec2 itemsRectMax = ImVec2(itemsRectMin.x + frameIndicesSize.x, itemsRectMin.y + frameIndicesSize.y);
			
			cursorPos = ImGui::GetCursorScreenPos();
			mousePos = ImGui::GetMousePos();
			mousePosRelative = ImVec2(ImGui::GetMousePos().x - cursorPos.x, ImGui::GetMousePos().y - cursorPos.y);

			isMouseInElementsRegion =
				mousePos.x >= itemsRectMin.x && mousePos.x < itemsRectMax.x &&
				mousePos.y >= itemsRectMin.y && mousePos.y < itemsRectMax.y;
			
			// Dragging/undragging the frame picker
			if ((isMouseInElementsRegion && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) || isPickerDragging)
			{
					s32 frameIndex = CLAMP((s32)(mousePosRelative.x / frameSize.x), 0, (f32)(animation->frameNum - 1));
					*self->time = frameIndex;

					isPickerDragging = true;
			}

			if (isPickerDragging && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
				isPickerDragging = false;

			// Draw the frame indices, based on the animation length
			for (s32 i = 0; i < animation->frameNum; i++)
			{
				ImVec2 imagePos = ImGui::GetCursorScreenPos();

				if (i % IMGUI_TIMELINE_FRAME_INDICES_MULTIPLE == 0)
				{
					std::string frameIndexString;
					frameIndexString = std::to_string(i);

					ImVec2 bgMin = imagePos;
					ImVec2 bgMax = ImVec2(imagePos.x + IMGUI_TIMELINE_FRAME_SIZE.x,
		                      imagePos.y + IMGUI_TIMELINE_FRAME_SIZE.y);

					ImU32 bgColor = ImGui::GetColorU32(IMGUI_FRAME_INDICES_OVERLAY_COLOR); 
					drawList->AddRectFilled(bgMin, bgMax, bgColor);

					ImVec2 textSize = ImGui::CalcTextSize(frameIndexString.c_str());

					ImVec2 textPos;
					textPos.x = imagePos.x + (IMGUI_TIMELINE_FRAME_SIZE.x - textSize.x) / 2.0f;
					textPos.y = imagePos.y + (IMGUI_TIMELINE_FRAME_SIZE.y - textSize.y) / 2.0f;

					drawList->AddText(textPos, ImGui::GetColorU32(ImGuiCol_Text), frameIndexString.c_str());
				}
				else
				{
					ImVec2 bgMin = imagePos;
					ImVec2 bgMax = ImVec2(imagePos.x + IMGUI_TIMELINE_FRAME_SIZE.x,
		                      imagePos.y + IMGUI_TIMELINE_FRAME_SIZE.y);

					ImU32 bgColor = ImGui::GetColorU32(IMGUI_FRAME_INDICES_COLOR); 
					drawList->AddRectFilled(bgMin, bgMax, bgColor);
				}

				ImGui::Image(self->resources->atlas.id, VEC2_TO_IMVEC2(ATLAS_SIZES[TEXTURE_FRAME]), IMVEC2_ATLAS_UV_GET(TEXTURE_FRAME));

				ImGui::SameLine();
			}

			ImGui::PopStyleVar();
			ImGui::PopStyleVar();

			pickerPos = ImVec2(cursorPos.x + *self->time * frameSize.x, cursorPos.y);
			lineStart = ImVec2(pickerPos.x + frameSize.x / 2.0f, pickerPos.y + frameSize.y);
			lineEnd = ImVec2(lineStart.x, lineStart.y + timelineSize.y - IMGUI_TIMELINE_FRAME_SIZE.y);

			ImGui::GetWindowDrawList()->AddImage
			(
				self->resources->atlas.id,
				pickerPos,
				ImVec2(pickerPos.x + frameSize.x, pickerPos.y + frameSize.y),
				IMVEC2_ATLAS_UV_GET(TEXTURE_PICKER)
			);
			
			drawList->AddRectFilled
			(
				ImVec2(lineStart.x - IMGUI_PICKER_LINE_SIZE, lineStart.y),
				ImVec2(lineStart.x + IMGUI_PICKER_LINE_SIZE, lineEnd.y),
				IMGUI_PICKER_LINE_COLOR
			);

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
			ImGui::EndChild();
			ImGui::PopClipRect();
			ImGui::PopStyleVar();
			ImGui::PopStyleVar();
		}
		else
		{
			ImGui::SameLine();
			ImGui::Dummy(frameIndicesSize);		
		}

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::BeginChild(STRING_IMGUI_TIMELINE_ELEMENT_LIST, IMGUI_TIMELINE_ELEMENT_LIST_SIZE, true,  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		ImGui::PopStyleVar();
		ImGui::PopStyleVar();
		ImGui::SetScrollY(itemScrollY);

		index = 0;

		// Root
		_imgui_timeline_item(self, Anm2Reference{animationID, ANM2_ROOT, 0, 0}, &index);

		// Layer 
		for (auto it = animation->layerAnimations.rbegin(); it != animation->layerAnimations.rend(); it++)
		{
			s32 id = it->first;
			_imgui_timeline_item(self, Anm2Reference{animationID, ANM2_LAYER, id, 0}, &index); 
		}

		// Null
		for (auto & [id, null] : animation->nullAnimations)
			_imgui_timeline_item(self, Anm2Reference{animationID, ANM2_NULL, id, 0}, &index); 

		// Triggers
		_imgui_timeline_item(self, Anm2Reference{animationID, ANM2_TRIGGERS, 0, 0}, &index); 

		// Swap the drag/drop elements
		if (self->isSwap)
		{
			Anm2Animation* animation = anm2_animation_from_reference(self->anm2, self->reference);

			switch (self->reference->itemType)
			{
				case ANM2_LAYER:
					map_swap(self->anm2->layers, self->reference->itemID, self->swapReference.itemID);
					map_swap(animation->layerAnimations, self->reference->itemID, self->swapReference.itemID);
					break;
				case ANM2_NULL:
					map_swap(self->anm2->nulls, self->reference->itemID, self->swapReference.itemID);
					map_swap(animation->nullAnimations, self->reference->itemID, self->swapReference.itemID);
					break;
				default:
					break;
			}

			self->isSwap = false;
			self->reference->itemID = self->swapReference.itemID;
			anm2_reference_clear(&self->swapReference);
		}

		ImGui::EndChild();
		ImGui::EndChild();

		// Add Element
		if (ImGui::Button(STRING_IMGUI_TIMELINE_ELEMENT_ADD))
			ImGui::OpenPopup(STRING_IMGUI_TIMELINE_ELEMENT_ADD_MENU);
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_ELEMENT_ADD);
		
		if (ImGui::BeginPopup(STRING_IMGUI_TIMELINE_ELEMENT_ADD_MENU))
		{
			if (ImGui::Selectable(STRING_IMGUI_TIMELINE_ELEMENT_ADD_MENU_LAYER))
				anm2_layer_add(self->anm2);
			_imgui_undoable(self);

			if (ImGui::Selectable(STRING_IMGUI_TIMELINE_ELEMENT_ADD_MENU_NULL))
				anm2_null_add(self->anm2);
			_imgui_undoable(self);

			ImGui::EndPopup();
		}

		ImGui::SameLine();

		// Remove Element
		if (ImGui::Button(STRING_IMGUI_TIMELINE_ELEMENT_REMOVE))
		{
			_imgui_undo_stack_push(self);
			
			switch (self->reference->itemType)
			{
				case ANM2_LAYER:
					anm2_layer_remove(self->anm2, self->reference->itemID);
					break;
				case ANM2_NULL:
					anm2_null_remove(self->anm2, self->reference->itemID);
					break;
				default:
					break;
			}

			anm2_reference_item_clear(self->reference);
		}
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_ELEMENT_REMOVE);

		ImGui::SameLine();

		// Play/Pause button
		if (ImGui::Button(buttonText))
		{
			self->preview->isPlaying = !self->preview->isPlaying;

			if ((s32)(*self->time) >= animation->frameNum - 1)
				*self->time = 0.0f;
		}
		_imgui_tooltip(buttonTooltipText);

		ImGui::SameLine();
		
		// Add frame
		if (ImGui::Button(STRING_IMGUI_TIMELINE_FRAME_ADD))
			anm2_frame_add(self->anm2, self->reference, (s32)*self->time);
		_imgui_undoable(self);
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_FRAME_ADD);

		ImGui::SameLine();

		// Remove Frame
		if (ImGui::Button(STRING_IMGUI_TIMELINE_FRAME_REMOVE))
		{
			Anm2Frame* frame = anm2_frame_from_reference(self->anm2, self->reference);

			if (frame)
			{
				_imgui_undo_stack_push(self);
				Anm2Item* item = anm2_item_from_reference(self->anm2, self->reference);
				item->frames.erase(item->frames.begin() + index);

				anm2_reference_frame_clear(self->reference);
			}
		}
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_FRAME_REMOVE);

		ImGui::SameLine();

		// Fit Animation Length
		if (ImGui::Button(STRING_IMGUI_TIMELINE_FIT_ANIMATION_LENGTH))
		{
			s32 length = anm2_animation_length_get(self->anm2, self->reference->animationID);
			animation->frameNum = length;
			*self->time = CLAMP(*self->time, 0.0f, animation->frameNum - 1);
		}
		_imgui_undoable(self);
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_FIT_ANIMATION_LENGTH);

		ImGui::SameLine();

		// Animation Length
		ImGui::SetNextItemWidth(IMGUI_TIMELINE_ANIMATION_LENGTH_WIDTH);
		ImGui::InputInt(STRING_IMGUI_TIMELINE_ANIMATION_LENGTH, &animation->frameNum);
		_imgui_undoable(self);
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_ANIMATION_LENGTH);

		animation->frameNum = CLAMP(animation->frameNum, ANM2_FRAME_NUM_MIN, ANM2_FRAME_NUM_MAX);

		ImGui::SameLine();

		// FPS
		ImGui::SetNextItemWidth(IMGUI_TIMELINE_FPS_WIDTH);
		ImGui::SameLine();
		ImGui::InputInt(STRING_IMGUI_TIMELINE_FPS, &self->anm2->fps);
		_imgui_undoable(self);
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_FPS);
		
		self->anm2->fps = CLAMP(self->anm2->fps, ANM2_FPS_MIN, ANM2_FPS_MAX);

		ImGui::SameLine();

		// Loop
		ImGui::Checkbox(STRING_IMGUI_TIMELINE_LOOP, &animation->isLoop);
		_imgui_undoable(self);
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_LOOP);
		ImGui::SameLine();

		// CreatedBy
		ImGui::SetNextItemWidth(IMGUI_TIMELINE_CREATED_BY_WIDTH);
		ImGui::SameLine();
		ImGui::InputText(STRING_IMGUI_TIMELINE_CREATED_BY, &self->anm2->createdBy[0], ANM2_STRING_MAX);
		_imgui_undoable(self);
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_TIMELINE_CREATED_BY);
		
		ImGui::SameLine();

		// CreatedOn
		ImGui::Text(STRING_IMGUI_TIMELINE_CREATED_ON, self->anm2->createdOn.c_str());
		ImGui::SameLine();

		// Version
		ImGui::Text(STRING_IMGUI_TIMELINE_VERSION, self->anm2->version);
	}

	ImGui::End();
}

// Taskbar
static void
_imgui_taskbar(Imgui* self)
{
	ImGuiWindowFlags taskbarWindowFlags = 0 |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse |
		ImGuiWindowFlags_NoSavedSettings;

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, IMGUI_TASKBAR_HEIGHT)); 

	ImGui::Begin(STRING_IMGUI_TASKBAR, NULL, taskbarWindowFlags);

	// File
	if (ImGui::Selectable(STRING_IMGUI_TASKBAR_FILE, false, 0, ImGui::CalcTextSize(STRING_IMGUI_TASKBAR_FILE)))
		ImGui::OpenPopup(STRING_IMGUI_FILE_MENU);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_FILE_MENU);

	if (ImGui::IsItemHovered() || ImGui::IsItemActive())
		ImGui::SetNextWindowPos(ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMin().y + ImGui::GetItemRectSize().y));
		
	if (ImGui::BeginPopup(STRING_IMGUI_FILE_MENU))
	{
		if (ImGui::Selectable(STRING_IMGUI_FILE_NEW))
		{
			anm2_reference_clear(self->reference);
			anm2_new(self->anm2);
		}
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_FILE_NEW);

		if (ImGui::Selectable(STRING_IMGUI_FILE_OPEN))
			dialog_anm2_open(self->dialog);
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_FILE_OPEN);

		if (ImGui::Selectable(STRING_IMGUI_FILE_SAVE))
		{
			// Open dialog if path empty, otherwise save in-place
			if (self->anm2->path.empty())
				dialog_anm2_save(self->dialog);
			else 
				anm2_serialize(self->anm2, self->anm2->path);
		}
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_FILE_SAVE);

		if (ImGui::Selectable(STRING_IMGUI_FILE_SAVE_AS))
			dialog_anm2_save(self->dialog);
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_FILE_SAVE_AS);

		ImGui::EndPopup();
	}

	ImGui::SameLine();

	// Playback
	if (ImGui::Selectable(STRING_IMGUI_TASKBAR_PLAYBACK, false, 0, ImGui::CalcTextSize(STRING_IMGUI_TASKBAR_PLAYBACK)))
		ImGui::OpenPopup(STRING_IMGUI_PLAYBACK_MENU);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_PLAYBACK_MENU);

	if (ImGui::IsItemHovered() || ImGui::IsItemActive())
		ImGui::SetNextWindowPos(ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMin().y + ImGui::GetItemRectSize().y));
		
	if (ImGui::BeginPopup(STRING_IMGUI_PLAYBACK_MENU))
	{
		ImGui::Checkbox(STRING_IMGUI_PLAYBACK_LOOP, &self->settings->playbackIsLoop);
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_PLAYBACK_IS_LOOP);
		ImGui::EndPopup();
	}

	ImGui::SameLine();

	// Wizard
	if (ImGui::Selectable(STRING_IMGUI_TASKBAR_WIZARD, false, 0, ImGui::CalcTextSize(STRING_IMGUI_TASKBAR_WIZARD)))
		ImGui::OpenPopup(STRING_IMGUI_WIZARD_MENU);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_WIZARD_MENU);

	if (ImGui::IsItemHovered() || ImGui::IsItemActive())
		ImGui::SetNextWindowPos(ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMin().y + ImGui::GetItemRectSize().y));
		
	if (ImGui::BeginPopup(STRING_IMGUI_WIZARD_MENU))
	{
		if (ImGui::Selectable(STRING_IMGUI_WIZARD_EXPORT_FRAMES_TO_PNG))
		{
			if (anm2_animation_from_reference(self->anm2, self->reference))
			{
				self->preview->isRecording = true;
				*self->time = true;
			}
		}
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_WIZARD_EXPORT_FRAMES_TO_PNG);
		ImGui::EndPopup();
	}

	// Add a persistent tooltip to indicate recording
	if (self->preview->isRecording)
	{
		ImVec2 mousePos = ImGui::GetMousePos();

		ImGui::SetNextWindowPos(ImVec2(mousePos.x + IMGUI_RECORD_TOOLTIP_OFFSET.x, mousePos.y + IMGUI_RECORD_TOOLTIP_OFFSET.y));
		ImGui::BeginTooltip();
		ImGui::Image(self->resources->atlas.id, VEC2_TO_IMVEC2(TEXTURE_SIZE), IMVEC2_ATLAS_UV_GET(TEXTURE_ANIMATION));
		ImGui::SameLine();
		ImGui::Text(STRING_IMGUI_RECORDING);
		ImGui::EndTooltip();
	}

	ImGui::End();
}

// Tools
static void
_imgui_tools(Imgui* self)
{
	ImGui::Begin(STRING_IMGUI_TOOLS);

	ImVec2 availableSize = ImGui::GetContentRegionAvail();
	f32 availableWidth = availableSize.x;

	s32 buttonsPerRow = availableWidth / TEXTURE_SIZE.x + IMGUI_TOOLS_WIDTH_INCREMENT;
	buttonsPerRow = MIN(buttonsPerRow, 1);

	for (s32 i = 0; i < TOOL_COUNT; i++)
	{
		const char* toolString;
		const char* toolTooltip;
		TextureType textureType;

		if (i > 0 && i % buttonsPerRow != 0)
			ImGui::SameLine();

		ImVec4 buttonColor = self->tool->type == (ToolType)i ? ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] : ImGui::GetStyle().Colors[ImGuiCol_Button];
		ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);

		switch (i)
		{
			case TOOL_PAN:
				toolString = STRING_IMGUI_TOOLS_PAN;
				toolTooltip = STRING_IMGUI_TOOLTIP_TOOLS_PAN;
				textureType = TEXTURE_PAN;
				break;
			case TOOL_MOVE:
				toolString = STRING_IMGUI_TOOLS_MOVE;
				toolTooltip = STRING_IMGUI_TOOLTIP_TOOLS_MOVE;
				textureType = TEXTURE_MOVE;
				break;
			case TOOL_ROTATE:
				toolString = STRING_IMGUI_TOOLS_ROTATE;
				toolTooltip = STRING_IMGUI_TOOLTIP_TOOLS_ROTATE;
				textureType = TEXTURE_ROTATE;
				break;
			case TOOL_SCALE:
				toolString = STRING_IMGUI_TOOLS_SCALE;
				toolTooltip = STRING_IMGUI_TOOLTIP_TOOLS_SCALE;
				textureType = TEXTURE_SCALE;
				break;
			case TOOL_CROP:
				toolString = STRING_IMGUI_TOOLS_CROP;
				toolTooltip = STRING_IMGUI_TOOLTIP_TOOLS_CROP;
				textureType = TEXTURE_CROP;
				break;
			default:
				break;
		}

		if (ImGui::ImageButton(toolString, self->resources->atlas.id, VEC2_TO_IMVEC2(ATLAS_SIZES[textureType]), IMVEC2_ATLAS_UV_GET(textureType)))
				self->tool->type = (ToolType)i;

		_imgui_tooltip(toolTooltip);

		ImGui::PopStyleColor();
	}

	ImGui::End();

}

// Animations
static void
_imgui_animations(Imgui* self)
{
	static s32 textEntryAnimationID = -1;

	ImGui::Begin(STRING_IMGUI_ANIMATIONS);

	// Iterate through all animations, can be selected and names can be edited
	for (auto & [id, animation] : self->anm2->animations)
	{
		std::string name;
		bool isSelected =  self->reference->animationID == id; 
		bool isTextEntry = textEntryAnimationID == id;

		// Distinguish default animation
		if (animation.name == self->anm2->defaultAnimation)
			name = std::format(STRING_IMGUI_ANIMATIONS_DEFAULT_ANIMATION_FORMAT, animation.name);
		else
			name = animation.name;

		ImGui::PushID(id);

		ImGui::Image(self->resources->atlas.id, VEC2_TO_IMVEC2(ATLAS_SIZES[TEXTURE_ANIMATION]), IMVEC2_ATLAS_UV_GET(TEXTURE_ANIMATION));
		ImGui::SameLine();

		if (isTextEntry)
		{
			if (ImGui::InputText(STRING_IMGUI_ANIMATIONS_ANIMATION_LABEL, &animation.name[0], ANM2_STRING_MAX, ImGuiInputTextFlags_EnterReturnsTrue))
				textEntryAnimationID = -1;
			_imgui_undoable(self);

			if (!ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
				textEntryAnimationID = -1;
		}
		else
		{
			if (ImGui::Selectable(name.c_str(), isSelected))
			{
				self->reference->animationID = id;
				anm2_reference_item_clear(self->reference);
				self->preview->isPlaying = false;
				*self->time = 0.0f;
			}

			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				textEntryAnimationID = id;

			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
			{
				ImGui::SetDragDropPayload(STRING_IMGUI_ANIMATIONS_DRAG_DROP, &id, sizeof(s32));
				ImGui::Image(self->resources->atlas.id, VEC2_TO_IMVEC2(ATLAS_SIZES[TEXTURE_ANIMATION]), IMVEC2_ATLAS_UV_GET(TEXTURE_ANIMATION));
				ImGui::SameLine();
				ImGui::Text(name.c_str());
				ImGui::EndDragDropSource();
			}

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(STRING_IMGUI_ANIMATIONS_DRAG_DROP))
				{
					s32 sourceID = *(s32*)payload->Data;
					if (sourceID != id)
					{
						_imgui_undo_stack_push(self);
						map_swap(self->anm2->animations, sourceID, id);
					}
				}
				ImGui::EndDragDropTarget();
			}
		}
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATIONS_SELECT);

		ImGui::PopID();
	}
	
	// Add
	if (ImGui::Button(STRING_IMGUI_ANIMATIONS_ADD))
	{
		bool isDefault = (s32)self->anm2->animations.size() == 0; // First animation is default automatically
		s32 id = anm2_animation_add(self->anm2);
		
		self->reference->animationID = id;

		if (isDefault)
			self->anm2->defaultAnimation = self->anm2->animations[id].name;
	}
	_imgui_undoable(self);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATIONS_ADD);
		
	ImGui::SameLine();

	Anm2Animation* animation = anm2_animation_from_reference(self->anm2, self->reference);

	if (animation)
	{
		// Remove
		if (ImGui::Button(STRING_IMGUI_ANIMATIONS_REMOVE))
		{
			anm2_animation_remove(self->anm2, self->reference->animationID);
			anm2_reference_clear(self->reference);
		}
		_imgui_undoable(self);
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATIONS_REMOVE);
		
		ImGui::SameLine();

		// Duplicate
		if (ImGui::Button(STRING_IMGUI_ANIMATIONS_DUPLICATE))
		{
			s32 id = map_next_id_get(self->anm2->animations);
			self->anm2->animations.insert({id, *animation});
			self->reference->animationID = id;
		}
		_imgui_undoable(self);
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATIONS_DUPLICATE);

		ImGui::SameLine();
		
		// Set as default
		if (ImGui::Button(STRING_IMGUI_ANIMATIONS_SET_AS_DEFAULT))
			self->anm2->defaultAnimation = animation->name;
		_imgui_undoable(self);
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATIONS_SET_AS_DEFAULT);
	}

	if (ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered() && ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
    	anm2_reference_clear(self->reference);

	ImGui::End();
}

// Events
static void
_imgui_events(Imgui* self)
{
	static s32 selectedEventID = -1;
	static s32 textEntryEventID = -1;

	ImGui::Begin(STRING_IMGUI_EVENTS);

	// Iterate through all events
	for (auto & [id, event] : self->anm2->events)
	{
		std::string eventString;
		bool isSelected = selectedEventID == id;
		bool isTextEntry = textEntryEventID == id;
		
		eventString = std::format(STRING_IMGUI_EVENT_FORMAT, id, event.name);

		ImGui::PushID(id);
		
		ImGui::Image(self->resources->atlas.id, VEC2_TO_IMVEC2(ATLAS_SIZES[TEXTURE_EVENT]), IMVEC2_ATLAS_UV_GET(TEXTURE_EVENT));
		ImGui::SameLine();

		if (isTextEntry)
		{
			if (ImGui::InputText(STRING_IMGUI_ANIMATIONS_ANIMATION_LABEL, &event.name[0], ANM2_STRING_MAX, ImGuiInputTextFlags_EnterReturnsTrue))
			{
				selectedEventID = -1;
				textEntryEventID = -1;
			}
			_imgui_undoable(self);

			if (!ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
				textEntryEventID = -1;
		}
		else
		{
			if (ImGui::Selectable(eventString.c_str(), isSelected))
				selectedEventID = id;

			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				textEntryEventID = id;

			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
			{
				ImGui::SetDragDropPayload(STRING_IMGUI_EVENTS_DRAG_DROP, &id, sizeof(s32));
				ImGui::Image(self->resources->atlas.id, VEC2_TO_IMVEC2(ATLAS_SIZES[TEXTURE_ANIMATION]), IMVEC2_ATLAS_UV_GET(TEXTURE_EVENT));
				ImGui::SameLine();
				ImGui::Text(eventString.c_str());
				ImGui::EndDragDropSource();
			}

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(STRING_IMGUI_EVENTS_DRAG_DROP))
				{
					s32 sourceID = *(s32*)payload->Data;
					if (sourceID != id)
					{
						_imgui_undo_stack_push(self);
						map_swap(self->anm2->events, sourceID, id);
					}
				}
				ImGui::EndDragDropTarget();
			}
		}
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_EVENTS_SELECT);

		ImGui::PopID();
	}
	
	if (ImGui::Button(STRING_IMGUI_EVENTS_ADD))
	{
		s32 id = map_next_id_get(self->anm2->events);
		self->anm2->events[id] = Anm2Event{}; 
		selectedEventID = id;
	}
	_imgui_undoable(self);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_EVENTS_ADD);
	
	ImGui::SameLine();
	
	if (selectedEventID != -1)
	{
		if (ImGui::Button(STRING_IMGUI_EVENTS_REMOVE))
		{
			self->anm2->events.erase(selectedEventID);
			selectedEventID = -1;
		}
		_imgui_undoable(self);
	}
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_EVENTS_REMOVE);

	if (ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered() && ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
		selectedEventID = -1;

	ImGui::End();
}

// Spritesheets
static void
_imgui_spritesheets(Imgui* self)
{
	static s32 selectedSpritesheetID = -1;

	ImGui::Begin(STRING_IMGUI_SPRITESHEETS);
	
	for (auto [id, spritesheet] : self->anm2->spritesheets)
	{
		ImVec2 spritesheetPreviewSize = IMGUI_SPRITESHEET_PREVIEW_SIZE;
		bool isSelected = selectedSpritesheetID == id; 
		Texture* texture = &self->resources->textures[id];
	
		std::string spritesheetString = std::format(STRING_IMGUI_SPRITESHEET_FORMAT, id, spritesheet.path);
	
		ImGui::BeginChild(spritesheetString.c_str(), IMGUI_SPRITESHEET_SIZE, true, ImGuiWindowFlags_None);
		
		ImGui::PushID(id);

		ImGui::Image(self->resources->atlas.id, VEC2_TO_IMVEC2(ATLAS_SIZES[TEXTURE_SPRITESHEET]), IMVEC2_ATLAS_UV_GET(TEXTURE_SPRITESHEET));
		ImGui::SameLine();

		if (ImGui::Selectable(spritesheetString.c_str(), isSelected))
		{
			selectedSpritesheetID = id;
			_imgui_spritesheet_editor_set(self, selectedSpritesheetID);
		}
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_SPRITESHEETS_SELECT);

		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
		{
			ImGui::SetDragDropPayload(STRING_IMGUI_SPRITESHEETS_DRAG_DROP, &id, sizeof(s32));
			ImGui::Image(self->resources->atlas.id, VEC2_TO_IMVEC2(ATLAS_SIZES[TEXTURE_SPRITESHEET]), IMVEC2_ATLAS_UV_GET(TEXTURE_SPRITESHEET));
			ImGui::SameLine();
			ImGui::Text(spritesheetString.c_str());
			ImGui::EndDragDropSource();
		}

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(STRING_IMGUI_SPRITESHEETS_DRAG_DROP))
			{
				s32 sourceID = *(s32*)payload->Data;
				if (sourceID != id)
				{
					map_swap(self->anm2->spritesheets, sourceID, id);
					map_swap(self->resources->textures, sourceID, id);
				}
			}
			ImGui::EndDragDropTarget();
		}

		f32 spritesheetAspect = (f32)self->resources->textures[id].size.x / self->resources->textures[id].size.y;

		if ((IMGUI_SPRITESHEET_PREVIEW_SIZE.x / IMGUI_SPRITESHEET_PREVIEW_SIZE.y) > spritesheetAspect)
			spritesheetPreviewSize.x = IMGUI_SPRITESHEET_PREVIEW_SIZE.y * spritesheetAspect;
		else
			spritesheetPreviewSize.y = IMGUI_SPRITESHEET_PREVIEW_SIZE.x / spritesheetAspect;

		if (texture->isInvalid)
			ImGui::Image(self->resources->atlas.id, VEC2_TO_IMVEC2(ATLAS_SIZES[TEXTURE_ERROR]), IMVEC2_ATLAS_UV_GET(TEXTURE_ERROR));
		else
			ImGui::Image(texture->id, spritesheetPreviewSize);
			
		ImGui::PopID();
			
		ImGui::EndChild();
	}

	if (ImGui::Button(STRING_IMGUI_SPRITESHEETS_ADD))
		dialog_png_open(self->dialog);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_SPRITESHEETS_ADD);
	
	ImGui::SameLine();

	// If spritesheet selected...
	if (selectedSpritesheetID > -1)
	{
		// Remove 
		if (ImGui::Button(STRING_IMGUI_SPRITESHEETS_REMOVE))
		{
			texture_free(&self->resources->textures[selectedSpritesheetID]);
			self->resources->textures.erase(selectedSpritesheetID);
			self->anm2->spritesheets.erase(selectedSpritesheetID);
			selectedSpritesheetID = -1;
		}
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_SPRITESHEETS_REMOVE);

		ImGui::SameLine();
		
		// Reload
		if (ImGui::Button(STRING_IMGUI_SPRITESHEETS_RELOAD))
		{
			// Save the working path, set path, to the anm2's path, load spritesheet, return to old path
			std::filesystem::path workingPath = std::filesystem::current_path();
			working_directory_from_file_set(self->anm2->path);
	
			resources_texture_init(self->resources, self->anm2->spritesheets[selectedSpritesheetID].path, selectedSpritesheetID);

			std::filesystem::current_path(workingPath);
		}
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_SPRITESHEETS_RELOAD);
		ImGui::SameLine();
		
		// Replace
		if (ImGui::Button(STRING_IMGUI_SPRITESHEETS_REPLACE))
		{
			self->dialog->replaceID = selectedSpritesheetID;
			dialog_png_replace(self->dialog);
		}
		_imgui_tooltip(STRING_IMGUI_TOOLTIP_SPRITESHEETS_REPLACE);
	}

	if (ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered() && ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
    	selectedSpritesheetID = -1;
		
	ImGui::End();
}

// Animation Preview
static void
_imgui_animation_preview(Imgui* self)
{
	static bool isPreviewHover = false;
	static bool isPreviewCenter = false;
	static vec2 mousePos = {0, 0};

	ImGui::Begin(STRING_IMGUI_ANIMATION_PREVIEW, NULL, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	
	ImVec2 windowSize = ImGui::GetWindowSize();

	// Grid settings
	ImGui::BeginChild(STRING_IMGUI_ANIMATION_PREVIEW_GRID_SETTINGS, IMGUI_ANIMATION_PREVIEW_SETTINGS_CHILD_SIZE, true);

	// Grid toggle
	ImGui::Checkbox(STRING_IMGUI_ANIMATION_PREVIEW_GRID, &self->settings->previewIsGrid);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATION_PREVIEW_GRID);

	ImGui::SameLine();
	
	// Grid Color
	ImGui::ColorEdit4(STRING_IMGUI_ANIMATION_PREVIEW_GRID_COLOR, (f32*)&self->settings->previewGridColorR, ImGuiColorEditFlags_NoInputs);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATION_PREVIEW_GRID_COLOR);
	
	// Grid Size
	ImGui::InputInt2(STRING_IMGUI_ANIMATION_PREVIEW_GRID_SIZE, (s32*)&self->settings->previewGridSizeX);
	self->settings->previewGridSizeX = CLAMP(self->settings->previewGridSizeX, PREVIEW_GRID_MIN, PREVIEW_GRID_MAX);
	self->settings->previewGridSizeY = CLAMP(self->settings->previewGridSizeY, PREVIEW_GRID_MIN, PREVIEW_GRID_MAX);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATION_PREVIEW_GRID_SIZE);
	
	// Grid Offset
	ImGui::InputInt2(STRING_IMGUI_ANIMATION_PREVIEW_GRID_OFFSET, (s32*)&self->settings->previewGridOffsetX);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATION_PREVIEW_GRID_OFFSET);
	ImGui::EndChild();
	
	ImGui::SameLine();
	
	ImGui::SameLine();
	
	// View settings
	ImGui::BeginChild(STRING_IMGUI_ANIMATION_PREVIEW_VIEW_SETTINGS, IMGUI_ANIMATION_PREVIEW_SETTINGS_CHILD_SIZE, true);

	// Zoom
	ImGui::DragFloat(STRING_IMGUI_ANIMATION_PREVIEW_ZOOM, &self->settings->previewZoom, 1, PREVIEW_ZOOM_MIN, PREVIEW_ZOOM_MAX, "%.0f");
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATION_PREVIEW_ZOOM);
	
	// Center view
	if (ImGui::Button(STRING_IMGUI_ANIMATION_PREVIEW_CENTER_VIEW))
		isPreviewCenter = true;
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATION_PREVIEW_CENTER_VIEW);
	
	// Mouse position; note that mousePos is relative to the animation preview and set later
	std::string mousePositionString = std::format(STRING_IMGUI_POSITION_FORMAT, (s32)mousePos.x, (s32)mousePos.y);
	ImGui::Text(mousePositionString.c_str());

	ImGui::EndChild();

	ImGui::SameLine();
	
	// Background settings
	ImGui::BeginChild(STRING_IMGUI_ANIMATION_PREVIEW_BACKGROUND_SETTINGS, IMGUI_ANIMATION_PREVIEW_SETTINGS_CHILD_SIZE, true);

	// Background color
	ImGui::ColorEdit4(STRING_IMGUI_ANIMATION_PREVIEW_BACKGROUND_COLOR, (f32*)&self->settings->previewBackgroundColorR, ImGuiColorEditFlags_NoInputs);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATION_PREVIEW_BACKGROUND_COLOR);
	
	ImGui::EndChild();

	ImGui::SameLine();

	// Helper settings
	ImGui::BeginChild(STRING_IMGUI_ANIMATION_PREVIEW_HELPER_SETTINGS, IMGUI_ANIMATION_PREVIEW_SETTINGS_CHILD_SIZE, true);
	
	// Axis toggle
	ImGui::Checkbox(STRING_IMGUI_ANIMATION_PREVIEW_AXIS, &self->settings->previewIsAxis);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATION_PREVIEW_AXIS);

	ImGui::SameLine();

	// Axis colors
	ImGui::ColorEdit4(STRING_IMGUI_ANIMATION_PREVIEW_AXIS_COLOR, (f32*)&self->settings->previewAxisColorR, ImGuiColorEditFlags_NoInputs);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATION_PREVIEW_AXIS_COLOR);
	
	// Root transform
	ImGui::Checkbox(STRING_IMGUI_ANIMATION_PREVIEW_ROOT_TRANSFORM, &self->settings->previewIsRootTransform);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATION_PREVIEW_ROOT_TRANSFORM);

	// Show pivot
	ImGui::Checkbox(STRING_IMGUI_ANIMATION_PREVIEW_SHOW_PIVOT, &self->settings->previewIsShowPivot);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_ANIMATION_PREVIEW_SHOW_PIVOT);

	ImGui::EndChild();

	// Animation preview texture
	vec2 previewPos = IMVEC2_TO_VEC2(ImGui::GetCursorPos());

	ImGui::Image(self->preview->texture, VEC2_TO_IMVEC2(PREVIEW_SIZE));

	self->preview->recordSize = vec2(windowSize.x, windowSize.y - IMGUI_ANIMATION_PREVIEW_SETTINGS_CHILD_SIZE.y);

	// Using tools when hovered
	if (ImGui::IsItemHovered())
	{
		vec2 windowPos = IMVEC2_TO_VEC2(ImGui::GetWindowPos());

		// Setting mouse pos relative to preview
		mousePos = IMVEC2_TO_VEC2(ImGui::GetMousePos());

		mousePos -= (windowPos + previewPos);
		mousePos -= (PREVIEW_SIZE / 2.0f);
		mousePos.x += self->settings->previewPanX;
		mousePos.y += self->settings->previewPanY;
		mousePos.x /= PERCENT_TO_UNIT(self->settings->previewZoom);
		mousePos.y /= PERCENT_TO_UNIT(self->settings->previewZoom);

		Anm2Frame* frame = anm2_frame_from_reference(self->anm2, self->reference);

		if (self->reference->itemType == ANM2_TRIGGERS) 
			frame = NULL;

		// Allow use of keybinds for tools
		self->tool->isEnabled = true;

		switch (self->tool->type)
		{
			case TOOL_PAN:
				SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER));
				
				if (mouse_held(&self->input->mouse, MOUSE_LEFT))
				{
					self->settings->previewPanX -= self->input->mouse.delta.x;
					self->settings->previewPanY -= self->input->mouse.delta.y;
				}
				break;
			case TOOL_MOVE:
				SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_MOVE));

				if (frame)
				{
					f32 step = input_held(self->input, INPUT_MOD) ? PREVIEW_MOVE_STEP_MOD : PREVIEW_MOVE_STEP;

					if 
					(
						mouse_press(&self->input->mouse, MOUSE_LEFT) ||
						input_press(self->input, INPUT_LEFT) ||
						input_press(self->input, INPUT_RIGHT) ||
						input_press(self->input, INPUT_UP) ||
						input_press(self->input, INPUT_DOWN)
					) 
						_imgui_undo_stack_push(self);

					if (mouse_held(&self->input->mouse, MOUSE_LEFT)) 
						frame->position = IMVEC2_TO_VEC2(mousePos);

					if (input_held(self->input, INPUT_LEFT)) 
						frame->position.x -= step;

					if (input_held(self->input, INPUT_RIGHT)) 
						frame->position.x += step;

					if (input_held(self->input, INPUT_UP))
						frame->position.y -= step;

					if (input_held(self->input, INPUT_DOWN)) 
						frame->position.y += step;
				}
				break;
			case TOOL_ROTATE:
				SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR));
				if (frame)
				{
					f32 step = input_held(self->input, INPUT_MOD) ? PREVIEW_ROTATE_STEP_MOD : PREVIEW_ROTATE_STEP;
					
					if 
					(
						mouse_press(&self->input->mouse, MOUSE_LEFT) ||
						input_press(self->input, INPUT_LEFT) ||
						input_press(self->input, INPUT_RIGHT) ||
						input_press(self->input, INPUT_UP) ||
						input_press(self->input, INPUT_DOWN)
					) 
						_imgui_undo_stack_push(self);

					if (mouse_held(&self->input->mouse, MOUSE_LEFT))
						frame->rotation += (s32)self->input->mouse.delta.x;

					if 
					(
						input_held(self->input, INPUT_LEFT) || 
						input_held(self->input, INPUT_UP)
					)
						frame->rotation -= step;

					if 
					(
						input_held(self->input, INPUT_RIGHT) || 
						input_held(self->input, INPUT_DOWN)
					)
						frame->rotation += step;
				}
				break;
			case TOOL_SCALE:
				SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NE_RESIZE));
				if (frame)
				{
					f32 step = input_held(self->input, INPUT_MOD) ? PREVIEW_SCALE_STEP_MOD : PREVIEW_SCALE_STEP;
					
					if 
					(
						mouse_press(&self->input->mouse, MOUSE_LEFT) ||
						input_press(self->input, INPUT_LEFT) ||
						input_press(self->input, INPUT_RIGHT) ||
						input_press(self->input, INPUT_UP) ||
						input_press(self->input, INPUT_DOWN)
					) 
						_imgui_undo_stack_push(self);

					if (input_held(self->input, INPUT_LEFT)) 
						frame->scale.x -= step;

					if (input_held(self->input, INPUT_RIGHT)) 
						frame->scale.x += step;

					if (input_held(self->input, INPUT_UP))
						frame->scale.y -= step;

					if (input_held(self->input, INPUT_DOWN)) 
						frame->scale.y += step;
						
					if (mouse_held(&self->input->mouse, MOUSE_LEFT)) 
					{
						frame->scale.x += (s32)self->input->mouse.delta.x;
						frame->scale.y += (s32)self->input->mouse.delta.y;
					}
				}
				break;
			case TOOL_CROP:
				SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR));
				break;
			default:
				break;
		};

		isPreviewHover = true;

		// Used to not be annoying when at lowest zoom
		self->settings->previewZoom = self->settings->previewZoom == EDITOR_ZOOM_MIN ? 0 : self->settings->previewZoom;

		// Zoom in 
		if (self->input->mouse.wheelDeltaY > 0 || input_release(self->input, INPUT_ZOOM_IN))
			self->settings->previewZoom += PREVIEW_ZOOM_STEP;

		// Zoom out 
		if (self->input->mouse.wheelDeltaY < 0 || input_release(self->input, INPUT_ZOOM_OUT))
			self->settings->previewZoom -= PREVIEW_ZOOM_STEP;

    	self->settings->previewZoom = CLAMP(self->settings->previewZoom, PREVIEW_ZOOM_MIN, PREVIEW_ZOOM_MAX);
	}
	else
	{
		if (isPreviewHover)
		{
			SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT));
			isPreviewHover = false;
		}
	}

	if (isPreviewCenter)
	{
		ImVec2 previewWindowRectSize = ImGui::GetCurrentWindow()->ClipRect.GetSize();

		// Based on the preview's crop in its window, adjust the pan
		self->settings->previewPanX = -(previewWindowRectSize.x - PREVIEW_SIZE.x) / 2.0f;
		self->settings->previewPanY = -((previewWindowRectSize.y - PREVIEW_SIZE.y) / 2.0f) + (IMGUI_ANIMATION_PREVIEW_SETTINGS_SIZE.y / 2.0f);

		isPreviewCenter = false;
	}

	ImGui::End();
}

// Spritesheet Editor
static void
_imgui_spritesheet_editor(Imgui* self)
{
	static bool isEditorHover = false;
	static bool isEditorCenter = false;
	static vec2 mousePos = {0, 0};

	ImGui::Begin(STRING_IMGUI_SPRITESHEET_EDITOR, NULL, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	
	// Grid settings
	ImGui::BeginChild(STRING_IMGUI_SPRITESHEET_EDITOR_GRID_SETTINGS, IMGUI_SPRITESHEET_EDITOR_SETTINGS_CHILD_SIZE, true);

	// Grid toggle
	ImGui::Checkbox(STRING_IMGUI_SPRITESHEET_EDITOR_GRID, &self->settings->editorIsGrid);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_SPRITESHEET_EDITOR_GRID);

	ImGui::SameLine();
	
	// Grid snap
	ImGui::Checkbox(STRING_IMGUI_SPRITESHEET_EDITOR_GRID_SNAP, &self->settings->editorIsGridSnap);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_SPRITESHEET_EDITOR_GRID_SNAP);

	ImGui::SameLine();

	// Grid color
	ImGui::ColorEdit4(STRING_IMGUI_SPRITESHEET_EDITOR_GRID_COLOR, (f32*)&self->settings->editorGridColorR, ImGuiColorEditFlags_NoInputs);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_SPRITESHEET_EDITOR_GRID_COLOR);
	
	// Grid size
	ImGui::InputInt2(STRING_IMGUI_SPRITESHEET_EDITOR_GRID_SIZE, (s32*)&self->settings->editorGridSizeX);
	self->settings->editorGridSizeX = CLAMP(self->settings->editorGridSizeX, PREVIEW_GRID_MIN, PREVIEW_GRID_MAX);
	self->settings->editorGridSizeY = CLAMP(self->settings->editorGridSizeY, PREVIEW_GRID_MIN, PREVIEW_GRID_MAX);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_SPRITESHEET_EDITOR_GRID_SIZE);
	
	// Grid offset
	ImGui::InputInt2(STRING_IMGUI_SPRITESHEET_EDITOR_GRID_OFFSET, (s32*)&self->settings->editorGridOffsetX);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_SPRITESHEET_EDITOR_GRID_OFFSET);

	ImGui::EndChild();
	
	ImGui::SameLine();
	
	// View settings
	ImGui::BeginChild(STRING_IMGUI_SPRITESHEET_EDITOR_VIEW_SETTINGS, IMGUI_SPRITESHEET_EDITOR_SETTINGS_CHILD_SIZE, true);

	// Zoom
	ImGui::DragFloat(STRING_IMGUI_SPRITESHEET_EDITOR_ZOOM, &self->settings->editorZoom, 1, PREVIEW_ZOOM_MIN, PREVIEW_ZOOM_MAX, "%.0f");
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_SPRITESHEET_EDITOR_ZOOM);
	
	// Center view
	if (ImGui::Button(STRING_IMGUI_SPRITESHEET_EDITOR_CENTER_VIEW))
		isEditorCenter = true;
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_SPRITESHEET_EDITOR_CENTER_VIEW);
	
	// Info position
	std::string mousePositionString = std::format(STRING_IMGUI_POSITION_FORMAT, (s32)mousePos.x, (s32)mousePos.y);
	ImGui::Text(mousePositionString.c_str());

	ImGui::EndChild();

	ImGui::SameLine();
	
	// Background settings
	ImGui::BeginChild(STRING_IMGUI_SPRITESHEET_EDITOR_BACKGROUND_SETTINGS, IMGUI_SPRITESHEET_EDITOR_SETTINGS_CHILD_SIZE, true);

	// Background color
	ImGui::ColorEdit4(STRING_IMGUI_SPRITESHEET_EDITOR_BACKGROUND_COLOR, (f32*)&self->settings->editorBackgroundColorR, ImGuiColorEditFlags_NoInputs);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_SPRITESHEET_EDITOR_BACKGROUND_COLOR);
	
	// Border
	ImGui::Checkbox(STRING_IMGUI_SPRITESHEET_EDITOR_BORDER, &self->settings->editorIsBorder);
	_imgui_tooltip(STRING_IMGUI_TOOLTIP_SPRITESHEET_EDITOR_BORDER);

	ImGui::EndChild();
	
	vec2 editorPos = IMVEC2_TO_VEC2(ImGui::GetCursorPos());
	ImGui::Image(self->editor->texture, VEC2_TO_IMVEC2(EDITOR_SIZE));
	
	// Panning and zoom
	if (ImGui::IsItemHovered())
	{
		vec2 windowPos = IMVEC2_TO_VEC2(ImGui::GetWindowPos());
		mousePos = IMVEC2_TO_VEC2(ImGui::GetMousePos());

		mousePos -= (windowPos + editorPos);
		mousePos -= (EDITOR_SIZE / 2.0f);
		mousePos.x += self->settings->editorPanX;
		mousePos.y += self->settings->editorPanY;
		mousePos.x /= PERCENT_TO_UNIT(self->settings->editorZoom);
		mousePos.y /= PERCENT_TO_UNIT(self->settings->editorZoom);

		isEditorHover = true;

		Anm2Frame* frame = anm2_frame_from_reference(self->anm2, self->reference);

		// Allow use of keybinds for tools
		self->tool->isEnabled = true;

		// Only changing layer frames
		if (self->reference->itemType != ANM2_LAYER)
			frame = NULL;

		if (self->tool->type == TOOL_CROP && frame)
		{
				SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR));

				if (mouse_press(&self->input->mouse, MOUSE_LEFT))
				{
					_imgui_undo_stack_push(self);
					
					vec2 cropPosition = mousePos + IMGUI_SPRITESHEET_EDITOR_CROP_FORGIVENESS;

					if (self->settings->editorIsGridSnap)
					{
						cropPosition.x = (s32)(cropPosition.x / self->settings->editorGridSizeX) * self->settings->editorGridSizeX;
						cropPosition.y = (s32)(cropPosition.y / self->settings->editorGridSizeX) * self->settings->editorGridSizeY;
					}

					frame->crop = cropPosition;
					frame->size = {0, 0};

				}
				else if (mouse_held(&self->input->mouse, MOUSE_LEFT))
				{
					vec2 sizePosition = mousePos + IMGUI_SPRITESHEET_EDITOR_CROP_FORGIVENESS;

					if (self->settings->editorIsGridSnap)
					{
						sizePosition.x = (s32)(sizePosition.x / self->settings->editorGridSizeX) * self->settings->editorGridSizeX;
						sizePosition.y = (s32)(sizePosition.y / self->settings->editorGridSizeX) * self->settings->editorGridSizeY;
					}

					frame->size = sizePosition - frame->crop;
				}
		}
		else
		{
			SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER));
			
			if (mouse_held(&self->input->mouse, MOUSE_LEFT))
			{
				self->settings->editorPanX -= self->input->mouse.delta.x;
				self->settings->editorPanY -= self->input->mouse.delta.y;
			}
		}

		// Used to not be annoying when at lowest zoom 
		self->settings->editorZoom = self->settings->editorZoom == EDITOR_ZOOM_MIN ? 0 : self->settings->editorZoom;
		
		// Zoom in
		if (self->input->mouse.wheelDeltaY > 0 || input_press(self->input, INPUT_ZOOM_IN))
			self->settings->editorZoom += PREVIEW_ZOOM_STEP;

		// Zoom out
		if (self->input->mouse.wheelDeltaY < 0 || input_press(self->input, INPUT_ZOOM_OUT))
			self->settings->editorZoom -= PREVIEW_ZOOM_STEP;

    	self->settings->editorZoom = CLAMP(self->settings->editorZoom, EDITOR_ZOOM_MIN, EDITOR_ZOOM_MAX);
	}
	else
	{
		if (isEditorHover)
		{
			SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT));
			isEditorHover = false;
		}
	}

	if (isEditorCenter)
	{
		self->settings->editorPanX = EDITOR_SIZE.x / 2.0f;
		self->settings->editorPanY = EDITOR_SIZE.y / 2.0f;

		isEditorCenter = false;
	}
	
	ImGui::End();
}

// Frame properties
static void
_imgui_frame_properties(Imgui* self)
{
	ImGui::Begin(STRING_IMGUI_FRAME_PROPERTIES);
	
	Anm2Frame* frame = anm2_frame_from_reference(self->anm2, self->reference);

	if (frame)
	{
		Anm2Animation* animation = anm2_animation_from_reference(self->anm2, self->reference);

		std::vector<const char*> eventNames;
		std::vector<s32> eventIDs;
		static s32 selectedEventIndex = -1;

		switch (self->reference->itemType)
		{
			case ANM2_ROOT:
				ImGui::Text(STRING_IMGUI_FRAME_PROPERTIES_ROOT);
				break;
			case ANM2_LAYER:
				ImGui::Text(STRING_IMGUI_FRAME_PROPERTIES_LAYER);
				break;
			case ANM2_NULL:
				ImGui::Text(STRING_IMGUI_FRAME_PROPERTIES_NULL);
				break;
			case ANM2_TRIGGERS:
				ImGui::Text(STRING_IMGUI_FRAME_PROPERTIES_TRIGGER);
				break;
			default:
				break;
		}

		switch (self->reference->itemType)
		{
			case ANM2_ROOT:
			case ANM2_NULL:
				// Position	
				ImGui::DragFloat2(STRING_IMGUI_FRAME_PROPERTIES_POSITION, value_ptr(frame->position), 1, 0, 0, "%.0f");
				_imgui_undoable(self);
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_POSITION);

				// Scale
				ImGui::DragFloat2(STRING_IMGUI_FRAME_PROPERTIES_SCALE, value_ptr(frame->scale), 1.0, 0, 0, "%.1f");
				_imgui_undoable(self);
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_SCALE);
				
				// Rotation
				ImGui::DragFloat(STRING_IMGUI_FRAME_PROPERTIES_ROTATION, &frame->rotation, 1, 0, 0, "%.1f");
				_imgui_undoable(self);
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_ROTATION);
				
				// Duration
				ImGui::InputInt(STRING_IMGUI_FRAME_PROPERTIES_DURATION, &frame->delay);
				_imgui_undoable(self);
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_DURATION);
				frame->delay = CLAMP(frame->delay, ANM2_FRAME_DELAY_MIN, animation->frameNum + 1);
				
				// Tint
				ImGui::ColorEdit4(STRING_IMGUI_FRAME_PROPERTIES_TINT, value_ptr(frame->tintRGBA));
				_imgui_undoable(self);
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_TINT);
				
				// Color Offset
				ImGui::ColorEdit3(STRING_IMGUI_FRAME_PROPERTIES_COLOR_OFFSET, value_ptr(frame->offsetRGB));
				_imgui_undoable(self);
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_COLOR_OFFSET);
				
				// Visible
				ImGui::Checkbox(STRING_IMGUI_FRAME_PROPERTIES_VISIBLE, &frame->isVisible);
				_imgui_undoable(self);
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_VISIBLE);
	
				ImGui::SameLine();
				
				// Interpolation
				ImGui::Checkbox(STRING_IMGUI_FRAME_PROPERTIES_INTERPOLATED, &frame->isInterpolated);
				_imgui_undoable(self);
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_INTERPOLATED);

				// Flip X
				if (ImGui::Button(STRING_IMGUI_FRAME_PROPERTIES_FLIP_X))
					frame->scale.x = -frame->scale.x;
				_imgui_undoable(self);
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_FLIP_X);
					
				ImGui::SameLine();
	
				// Flip Y
				if (ImGui::Button(STRING_IMGUI_FRAME_PROPERTIES_FLIP_Y))
					frame->scale.y = -frame->scale.y;
				_imgui_undoable(self);
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_FLIP_Y);
				break;
			case ANM2_LAYER:
				// Position	
				ImGui::DragFloat2(STRING_IMGUI_FRAME_PROPERTIES_POSITION, value_ptr(frame->position), 1, 0, 0, "%.0f");
				_imgui_undoable(self);
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_POSITION);

				// Crop Position
				ImGui::DragFloat2(STRING_IMGUI_FRAME_PROPERTIES_CROP_POSITION, value_ptr(frame->crop), 1, 0, 0, "%.0f");
				_imgui_undoable(self);
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_CROP_POSITION);
				
				// Crop
				ImGui::DragFloat2(STRING_IMGUI_FRAME_PROPERTIES_CROP_SIZE, value_ptr(frame->size), 1, 0, 0, "%.0f");
				_imgui_undoable(self);
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_CROP_SIZE);
				
				// Pivot
				ImGui::DragFloat2(STRING_IMGUI_FRAME_PROPERTIES_PIVOT, value_ptr(frame->pivot), 1, 0, 0, "%.0f");
				_imgui_undoable(self);
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_PIVOT);

				// Scale
				ImGui::DragFloat2(STRING_IMGUI_FRAME_PROPERTIES_SCALE, value_ptr(frame->scale), 1.0, 0, 0, "%.1f");
				_imgui_undoable(self);
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_SCALE);
				
				// Rotation
				ImGui::DragFloat(STRING_IMGUI_FRAME_PROPERTIES_ROTATION, &frame->rotation, 1, 0, 0, "%.1f");
				_imgui_undoable(self);
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_ROTATION);
				
				// Duration
				ImGui::InputInt(STRING_IMGUI_FRAME_PROPERTIES_DURATION, &frame->delay);
				_imgui_undoable(self);
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_DURATION);
				// clamp delay
				frame->delay = CLAMP(frame->delay, ANM2_FRAME_DELAY_MIN, animation->frameNum + 1);
				
				// Tint
				ImGui::ColorEdit4(STRING_IMGUI_FRAME_PROPERTIES_TINT, value_ptr(frame->tintRGBA));
				_imgui_undoable(self);
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_TINT);
				
				// Color Offset
				ImGui::ColorEdit3(STRING_IMGUI_FRAME_PROPERTIES_COLOR_OFFSET, value_ptr(frame->offsetRGB));
				_imgui_undoable(self);
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_COLOR_OFFSET);
	
				// Flip X
				if (ImGui::Button(STRING_IMGUI_FRAME_PROPERTIES_FLIP_X))
					frame->scale.x = -frame->scale.x;
				_imgui_undoable(self);
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_FLIP_X);
					
				ImGui::SameLine();
	
				// Flip Y
				if (ImGui::Button(STRING_IMGUI_FRAME_PROPERTIES_FLIP_Y))
					frame->scale.y = -frame->scale.y;
				_imgui_undoable(self);
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_FLIP_Y);
				
				// Visible
				ImGui::Checkbox(STRING_IMGUI_FRAME_PROPERTIES_VISIBLE, &frame->isVisible);
				_imgui_undoable(self);
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_VISIBLE);
				
				ImGui::SameLine();
				
				// Interpolation
				ImGui::Checkbox(STRING_IMGUI_FRAME_PROPERTIES_INTERPOLATED, &frame->isInterpolated);
				_imgui_undoable(self);
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_INTERPOLATED);
				break;
			case ANM2_TRIGGERS:
				// Events drop down; pick one!
				for (auto & [id, event] : self->anm2->events) 
				{
					eventIDs.push_back(id);
					eventNames.push_back(event.name.c_str());
					if (id == frame->eventID)
						selectedEventIndex = eventIDs.size() - 1;
				}
				
				if (ImGui::Combo(STRING_IMGUI_FRAME_PROPERTIES_EVENT, &selectedEventIndex, eventNames.data(), eventNames.size())) 
				{
    				frame->eventID = eventIDs[selectedEventIndex];
					selectedEventIndex = -1;
				}
				_imgui_undoable(self);
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_EVENT);

				// At Frame
				ImGui::InputInt(STRING_IMGUI_FRAME_PROPERTIES_AT_FRAME, &frame->atFrame);
				_imgui_undoable(self);
				_imgui_tooltip(STRING_IMGUI_TOOLTIP_FRAME_PROPERTIES_AT_FRAME);
				frame->atFrame = CLAMP(frame->atFrame, 0, animation->frameNum- 1);
				break;
			default:
				break;
		}
	
	}

	ImGui::End();
}

// Initializes imgui
void 
imgui_init
(
    Imgui* self,
    Dialog* dialog,
    Resources* resources,
    Input* input,
    Anm2* anm2,
    Anm2Reference* reference,
	f32* time,
    Editor* editor,
    Preview* preview,
    Settings* settings,
    Tool* tool,
    Snapshots* snapshots,
    SDL_Window* window,
    SDL_GLContext* glContext
)
{
	IMGUI_CHECKVERSION();

	self->dialog = dialog;
	self->resources = resources;
	self->input = input;
	self->anm2 = anm2;
	self->reference = reference;
	self->time = time;
	self->editor = editor;
	self->preview = preview;
	self->settings = settings;
	self->tool = tool;
	self->snapshots = snapshots;
	self->window = window;
	self->glContext = glContext;

	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGui_ImplSDL3_InitForOpenGL(self->window, *self->glContext);
	ImGui_ImplOpenGL3_Init(STRING_OPENGL_VERSION);

	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = NULL;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigWindowsMoveFromTitleBarOnly = true;

	ImGui::LoadIniSettingsFromDisk(PATH_SETTINGS);
	
	std::cout << STRING_INFO_IMGUI_INIT << std::endl;
}

// Main dockspace
static void
_imgui_dock(Imgui* self)
{
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	
	ImGuiWindowFlags dockspaceWindowFlags = 0 |
		ImGuiWindowFlags_NoTitleBar | 
		ImGuiWindowFlags_NoCollapse | 
		ImGuiWindowFlags_NoResize | 
		ImGuiWindowFlags_NoMove | 
		ImGuiWindowFlags_NoBringToFrontOnFocus | 
		ImGuiWindowFlags_NoNavFocus;

    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + IMGUI_TASKBAR_HEIGHT));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, viewport->Size.y - IMGUI_TASKBAR_HEIGHT));
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::Begin(STRING_IMGUI_WINDOW, NULL, dockspaceWindowFlags);

    ImGui::DockSpace(ImGui::GetID(STRING_IMGUI_DOCKSPACE), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

	_imgui_tools(self);
	_imgui_animations(self);
	_imgui_events(self);
	_imgui_spritesheets(self);
	_imgui_animation_preview(self);
	_imgui_spritesheet_editor(self);
	_imgui_timeline(self);
	_imgui_frame_properties(self);
	
	ImGui::End();
}

// Ticks imgui
void
imgui_tick(Imgui* self)
{
	ImGui_ImplSDL3_NewFrame();
	ImGui_ImplOpenGL3_NewFrame();
	ImGui::NewFrame();

	self->tool->isEnabled = false;

	_imgui_taskbar(self);
	_imgui_dock(self);
}

// Draws imgui
void
imgui_draw(void)
{
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// Frees imgui
void
imgui_free(void)
{
	ImGui_ImplSDL3_Shutdown();
	ImGui_ImplOpenGL3_Shutdown();

	// Save Ini manually
	ImGui::SaveIniSettingsToDisk(PATH_SETTINGS);
	ImGui::DestroyContext();

	std::cout << STRING_INFO_IMGUI_FREE << std::endl;
}
