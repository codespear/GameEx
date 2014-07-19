#define BOOST_TEST_NO_DYN_LINK
#include <boost/test/unit_test.hpp>
#include "../scenenode.h"
using namespace codespear;

struct TestNode : public SceneNode {
	bool drawn = false;
};

struct Fixture {
	SceneNode root;
	sf::RenderTexture target;
	sf::RenderStates states;
	TestNode * add_node() {
		auto r = new TestNode();
		root.attach(SceneNode::u_ptr(r));
		return r;
	}
	Fixture() {
		target.create(100,100);
	}
};

BOOST_AUTO_TEST_SUITE( SceneNodeSuite )

BOOST_FIXTURE_TEST_CASE( attach_and_detach, Fixture ) {
	auto child_p = add_node();
	auto p = root.detach(child_p);
	BOOST_REQUIRE_MESSAGE(p,"detach returned an empty node");
	BOOST_REQUIRE(p.get() == child_p);
}

BOOST_FIXTURE_TEST_CASE( draw, Fixture ) {
	auto child_p = add_node();
//	root.draw(target,states);
	BOOST_REQUIRE(child_p->drawn);
}


BOOST_AUTO_TEST_SUITE_END()