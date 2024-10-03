#ifndef GAME_EDITOR_MAP_GRID_H
#define GAME_EDITOR_MAP_GRID_H

#include "component.h"

class CMapGrid : public CEditorComponent
{
public:
	void OnReset() override;
	void OnRender(CUIRect View) override;

	void SnapToGrid(float &x, float &y) const;
	int GridLineDistance() const;

	/**
	 * Returns whether the grid is rendered.
	 */
	bool IsEnabled() const;

	void Toggle();

	int Factor() const;
	void ResetFactor();
	void IncreaseFactor();
	void DecreaseFactor();

private:
	bool m_GridActive;
	int m_GridFactor;
};

#endif
