#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include "../derivation.h"
#include <algorithm>

using namespace pg;
namespace bt = boost::unit_test;

BOOST_AUTO_TEST_SUITE(derivation)

BOOST_AUTO_TEST_CASE(test_get_names)
{
	DerivationTree tree;
	tree.createRoot();
	tree.addSibling("1");
	tree.addChild("2");
	tree.addSibling("2.1");
	tree.addSibling("2.1");
	auto names = tree.getNames();
	auto first = names.begin();
	auto last = names.end();
	BOOST_TEST(names.size() == 5);
	BOOST_TEST((std::find(first, last, "1") != last));
	BOOST_TEST((std::find(first, last, "2") != last));
	BOOST_TEST((std::find(first, last, "2.1") != last));
	BOOST_TEST((std::find(first, last, "2.2") != last));
	BOOST_TEST((std::find(first, last, "2.3") != last));

	tree.remove("2.1");
	names = tree.getNames();
	first = names.begin();
	last = names.end();
	BOOST_TEST(names.size() == 4);
	BOOST_TEST((std::find(first, last, "1") != last));
	BOOST_TEST((std::find(first, last, "2") != last));
	BOOST_TEST((std::find(first, last, "2.1") != last));
	BOOST_TEST((std::find(first, last, "2.2") != last));

	tree.remove("2");
	names = tree.getNames();
	BOOST_TEST(names.size() == 1);
	BOOST_TEST(names[0] == "1");

	tree.reset();
	names = tree.getNames();
	BOOST_TEST(names.empty());
}

BOOST_AUTO_TEST_CASE(test_assignment)
{
	DerivationTree tree;
	tree.createRoot();
	tree.addChild("1");
	tree.addChild("1.1");
	tree.addSibling("1");
	tree.addChild("2");
	DerivationTree treeCopy;
	treeCopy.createRoot();
	treeCopy.addChild("1");
	treeCopy.addSibling("1.1");
	treeCopy = tree;

	BOOST_TEST(tree.getRoot() != treeCopy.getRoot());

	auto names = treeCopy.getNames();
	auto first = names.begin();
	auto last = names.end();
	BOOST_TEST(names.size() == 5);
	BOOST_TEST((std::find(first, last, "1") != last));
	BOOST_TEST((std::find(first, last, "1.1") != last));
	BOOST_TEST((std::find(first, last, "1.1.1") != last));
	BOOST_TEST((std::find(first, last, "2") != last));
	BOOST_TEST((std::find(first, last, "2.1") != last));
}

BOOST_AUTO_TEST_CASE(test_nonexistent)
{
	DerivationTree tree;
	tree.createRoot();
	BOOST_TEST(tree.addChild("2") == nullptr);
	BOOST_TEST(tree.addSibling("1.1") == nullptr);
	BOOST_TEST(!tree.remove("3.2"));
}

BOOST_AUTO_TEST_CASE(test_structure)
{
	DerivationTree tree;
	DerivationNode *node1 = tree.createRoot();
	DerivationNode *node2 = tree.addSibling("1");
	DerivationNode *node1_1 = tree.addChild("1");
	DerivationNode *node1_2 = tree.addSibling("1.1");

	BOOST_TEST(node2->getPrevSibling() == node1);
	BOOST_TEST(node2->getNextSibling() == nullptr);

	BOOST_TEST(node1_1->getPrevSibling() == nullptr);
	BOOST_TEST(node1_1->getNextSibling() == node1_2);
	BOOST_TEST(node1_1->getParent() == node1);
	BOOST_TEST(node1_1->getChild() == nullptr);

	BOOST_TEST(node1_2->getPrevSibling() == node1_1);
	BOOST_TEST(node1_2->getNextSibling() == nullptr);
	BOOST_TEST(node1_2->getParent() == node1);
	BOOST_TEST(node1_2->getChild() == nullptr);
}

BOOST_AUTO_TEST_CASE(test_remove_second_sibling)
{
	DerivationTree tree;
	tree.createRoot();
	tree.addChild("1");
	tree.addSibling("1.1");
	BOOST_TEST(tree.remove("1.2"));
	DerivationNode *node = tree.getRoot();
	BOOST_TEST(node);
	BOOST_TEST(node->getChild());
	BOOST_TEST(node->getChild()->getParent() == node);
	BOOST_TEST(!node->getChild()->getChild());
	BOOST_TEST(!node->getChild()->getNextSibling());
	BOOST_TEST(!node->getChild()->getPrevSibling());
}

BOOST_AUTO_TEST_CASE(test_remove_first_sibling)
{
	DerivationTree tree;
	tree.createRoot();
	tree.addChild("1");
	tree.addSibling("1.1");
	BOOST_TEST(tree.remove("1.1"));
	DerivationNode *node = tree.getRoot();
	BOOST_TEST(node);
	BOOST_TEST(node->getChild());
	BOOST_TEST(node->getChild()->getParent() == node);
	BOOST_TEST(!node->getChild()->getChild());
	BOOST_TEST(!node->getChild()->getNextSibling());
	BOOST_TEST(!node->getChild()->getPrevSibling());
}

BOOST_AUTO_TEST_SUITE_END()
