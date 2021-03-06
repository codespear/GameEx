/**
 * Copyright 2012 Willem Duminy
 * See LICENCE.txt
 */

#pragma once
#include <game_objects.h>
#include <font.h>
 
using namespace game;

class FrontPage : public GameObjectChainLink {
	PREVENT_COPY(FrontPage)
public:
	FrontPage();
	void initialise(const ResourceContext &rc, const DrawContext& dc) override; 
	void update(const GameContext &uc) override;
	bool escaped() const {return _escaped;}
	void set_active(const bool v) override {GameObjectChainLink::set_active(v);set_hidden(!v);}
private:
	Font2D::u_ptr _font;
	Panel * _panel;
	bool _escaped;
};
