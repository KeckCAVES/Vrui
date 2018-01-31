/***********************************************************************
Trie - Class to store a dictionary of attributed words as a trie
structure for fast prefix matching and attribute retrieval.
Copyright (c) 2018 Oliver Kreylos
***********************************************************************/

#ifndef MISC_TRIE_INCLUDED
#define MISC_TRIE_INCLUDED

namespace Misc {

template <class CharParam,CharParam terminatorParam,class LeafDataParam>
class Trie
	{
	/* Embedded classes: */
	public:
	typedef CharParam Char; // Data type for a single character
	typedef LeafDataParam LeafData; // Data type to associate data with leaf trie nodes
	static CharParam terminator=terminatorParam; // String terminator character
	
	template <class DataParam>
	class TrieLeaf // Generic helper class to represent data associated with trie leaves
		{
		/* Embedded classes: */
		public:
		typedef DataParam Data; // Type of data stored in the trie leaf
		
		/* Elements: */
		Data data; // Data
		
		/* Methods: */
		void set(DataParam newData)
			{
			data=newData;
			}
		void destroy(void)
			{
			/* Nothing to destroy... */
			}
		};
	
	template <>
	class TrieLeaf<void> // Specialized helper class for trie leaves that don't store data
		{
		/* Methods: */
		public:
		void destroy(void)
			{
			/* Nothing to destroy... */
			}
		};
	
	template <class DataParam>
	class TrieLeaf<DataParam*> // Specialized helper class for trie leaves that store pointers to data
		{
		/* Embedded classes: */
		public:
		typedef DataParam Data; // Type of data pointed to by the trie leaf
		
		/* Elements: */
		Data* data; // Pointer to data
		
		/* Methods: */
		void destroy(void)
			{
			/* Destroy the pointed-to data: */
			delete data;
			}
		};
	
	struct Node // Structure representing trie nodes
		{
		/* Embedded classes: */
		public:
		struct Suffix // Structure representing a sub-trie and the character associated with its root
			{
			/* Elements: */
			public:
			Char character; // The character associated with the sub-trie's root
			union
				{
				Node* subtree; // Pointer to the sub-trie's root node
				TreeLeaf<LeafDataParam> leaf; // Leaf data associated with a node
				};
			};
		
		/* Elements: */
		size_t suffixArraySize; // Allocated size of the suffix array
		Suffix* suffixArray; // Suffix array, sorted by character value for binary search
		size_t numSuffixes; // Number of suffixes currently in the array
		
		/* Constructors and destructors: */
		Node(void) // Creates a node with a small empty suffix array
			:suffixArraySize(5),suffixArray(new Suffix[suffixArraySize]),
			 numSuffixes(0)
			{
			}
		~Node(void) // Destroys the node and its subtree
			{
			for(size_t i=0;i<numSuffixes;++i)
				{
				if(suffixArray[i].character!=terminator)
					{
					/* Delete the sub-trie: */
					delete suffixArray[i].subtree;
					}
				else
					{
					/* Destroy the node's associated leaf data: */
					suffixArray[i].leaf.destroy();
					}
				}
			}
		};
	
	/* Elements: */
	private:
	Node root; // Trie's root node
	
	/* Constructors and destructors: */
	public:
	Trie(void) // Creates an empty trie
		{
		}
	private:
	Trie(const Trie& source); // Prohibit copy constructor
	Trie& operator=(const Trie& source); // Prohibit assignment operator
	public:
	~Trie(void) // Destroys the trie
		{
		}
	
	/* Methods: */
	bool addWord(const Char* word,LeafData leaf); // Adds a word to the trie and associates it with the given data; replaces word's leaf data and returns false if the word was already in the tree
	bool isPrefix(const Char* word) const; // Returns true if the given word is a prefix of some word in the trie
	const Leaf* isWord(const Char* word) const; // Returns the leaf data associated with the given word if it is in the trie, null otherwise
	bool removeWord(const Char* word); // Removes the given word from the tree; returns false if the word wasn't in the trie
	};

}

#ifndef MISC_TRIE_IMPLEMENTATION
#include <Misc/Trie.icpp>
#endif

#endif
