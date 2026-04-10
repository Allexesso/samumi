#ifndef WHODUN_TREE_H
#define WHODUN_TREE_H 1

#include <vector>

#include "whodun_cache.h"

namespace whodun{

class Tree;

/**A representation of a node in an immutable tree.*/
class TreeNode{
public:
	/**Allow subclass cleanup.*/
	virtual ~TreeNode();
	/**
	 * Get the payload of this node.
	 * @return This node's payload.
	 */
	virtual void* getPayload() = 0;
	/**
	 * Get the number of children of this node.
	 * @return The number of children.
	 */
	virtual uintptr_t numChildren() = 0;
	/**
	 * Get a child.
	 * @param indC The child index.
	 * @return The requested child.
	 */
	virtual TreeNode* getChild(uintptr_t indC) = 0;
	/**
	 * Get the parent of this thing.
	 * @return The parent: null if none.
	 */
	virtual TreeNode* getParent() = 0;
};

/**An immutable tree.*/
class Tree{
public:
	/**Allow subclass cleanup.*/
	virtual ~Tree();
	/**
	 * Get the root node.
	 * @return The token for the node.
	 */
	virtual TreeNode* getRoot() = 0;
};

//TODO mutable tree node

/**Figure out how to partition data, and note which way to go at a junction.*/
class Partitioner{
public:
	/**Allow subclass cleanup.*/
	virtual ~Partitioner();
	/**
	 * Get the size of a split structure.
	 * @return The size of a split structure.
	 */
	virtual uintptr_t splitSize() = 0;
	/**
	 * Decide how to split data.
	 * @param numData The number of data to split.
	 * @param theData The data in question.
	 * @param splStore Storage for the split data.
	 * @return Whether a split should be made.
	 */
	virtual int allocateSplit(uintptr_t numData, void** theData, void* splStore) = 0;
	/**
	 * Decide which side of a line a datapoint lives on.
	 * @param forSpl The line.
	 * @param toFollow The data.
	 * @return Whether it's on the negative or positive child. 0 for both.
	 */
	virtual int splitDir(void* forSpl, void* toFollow) = 0;
	//TODO
};

class PartitionTree;

/**A node in a partition tree.*/
class PartitionTreeNode : public TreeNode{
public:
	/**The index of the parent.*/
	uintptr_t parI;
	/**The index of the left sub node.*/
	uintptr_t leftSubI;
	/**The index of the right sub node.*/
	uintptr_t rightSubI;
	/**My tree.*/
	PartitionTree* myTree;
	/**The split offset.*/
	uintptr_t splitI;
	/**The number of things this node stores.*/
	uintptr_t numData;
	/**My data index.*/
	uintptr_t myDInd;
	
	void* getPayload();
	uintptr_t numChildren();
	TreeNode* getChild(uintptr_t indC);
	TreeNode* getParent();
};

/**A tree built to partition a static set of data.*/
class PartitionTree : public Tree{
public:
	/**
	 * Set up an empty partition tree.
	 * @param toPartition The partition method.
	 */
	PartitionTree(Partitioner* toPartition);
	/**Tear down.*/
	~PartitionTree();
	/**
	 * Clear the current tree and build a new one.
	 * @param numData The amount of data.
	 * @param theData The data to build on.
	 */
	void buildTree(uintptr_t numData, void** theData);
	/**The partition method*/
	Partitioner* toPart;
	/**The nodes in the built tree.*/
	std::vector<PartitionTreeNode> allNodes;
	/**The split info.*/
	std::vector<char> allSplits;
	/**The partitioned data.*/
	std::vector<void*> partData;
	/**A temporary stack.*/
	std::vector< triple<uintptr_t,uintptr_t,uintptr_t> > tmpStack;
	
	TreeNode* getRoot();
};

}

#endif