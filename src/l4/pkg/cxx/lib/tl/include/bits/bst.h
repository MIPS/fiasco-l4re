// vi:ft=cpp
/**
 * \file
 * \brief AVL tree
 */
/*
 * (c) 2008-2009 Alexander Warg <warg@os.inf.tu-dresden.de>,
 *               Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 *
 * As a special exception, you may use this file as part of a free software
 * library without restriction.  Specifically, if other files instantiate
 * templates or use macros or inline functions from this file, or you compile
 * this file and link it with other files to produce an executable, this
 * file does not by itself cause the resulting executable to be covered by
 * the GNU General Public License.  This exception does not however
 * invalidate any other reasons why the executable file might be covered by
 * the GNU General Public License.
 */
#pragma once

#include "type_traits.h"
#include "bst_base.h"
#include "bst_iter.h"

namespace cxx { namespace Bits {

/**
 * \brief Basic binary search tree (BST).
 *
 * This class is intended as a base class for concrete binary search trees,
 * such as an AVL tree.  This class already provides the basic lookup methods
 * and iterator definitions for a BST.
 */
template< typename Node, typename Get_key, typename Compare >
class Bst
{
private:
  typedef Direction Dir;
  /// Ops for forward iterators
  struct Fwd
  {
    static Node *child(Node const *n, Direction d)
    { return Bst_node::next<Node>(n, d); }

    static bool cmp(Node const *l, Node const *r)
    { return Compare()(Get_key::key_of(l), Get_key::key_of(r)); }
  };

  /// Ops for Reverse iterators
  struct Rev
  {
    static Node *child(Node const *n, Direction d)
    { return Bst_node::next<Node>(n, !d); }

    static bool cmp(Node const *l, Node const *r)
    { return Compare()(Get_key::key_of(r), Get_key::key_of(l)); }
  };

public:
  /// The type of key values used to generate the total order of the elements.
  typedef typename Get_key::Key_type Key_type;
  /// The type for key parameters.
  typedef typename Type_traits<Key_type>::Param_type Key_param_type;

  /// Helper for building forward iterators for different wrapper classes.
  typedef Fwd Fwd_iter_ops;
  /// Helper for building reverse iterators for different wrapper classes.
  typedef Rev Rev_iter_ops;

  /// \name Iterators
  //@{
  /// Forward iterator.
  typedef __Bst_iter<Node, Node, Fwd> Iterator;
  /// Constant forward iterator.
  typedef __Bst_iter<Node, Node const, Fwd> Const_iterator;
  /// Backward iterator.
  typedef __Bst_iter<Node, Node, Rev> Rev_iterator;
  /// Constant backward.
  typedef __Bst_iter<Node, Node const, Rev> Const_rev_iterator;
  //@}

protected:
  /**
   * \name Interior access for descendants.
   *
   * As this class is an intended base class we provide protected access
   * to our interior, use 'using' to make this private in concrete
   * implementations.
   */
  /*@{*/

  /// The head pointer of the tree.
  Bst_node *_head;

  /// Create an empty tree.
  Bst() : _head(0) {}

  /// Access the head node as object of type \a Node.
  Node *head() const { return static_cast<Node*>(_head); }

  /// Get the key value of \a n.
  static Key_type k(Bst_node const *n)
  { return Get_key::key_of(static_cast<Node const *>(n)); }

  /**
   * \brief Get the direction to go from \a l to search for \a r.
   * \param l is the key to look for.
   * \param r is the key at the current position.
   * \return #Direction::L for left, #Direction::R for right,
   *         and #Direction::N if \a l is equal to \a r.
   */
  static Dir dir(Key_param_type l, Key_param_type r)
  {
    Compare cmp;
    Dir d(cmp(r, l));
    if (d == Direction::L && !cmp(l, r))
      return Direction::N;
    return d;
  }

  /**
   * \brief Get the direction to go from \a l to search for \a r.
   * \param l is the key to look for.
   * \param r is the node at the current position.
   * \return #Direction::L for left, #Direction::R for right,
   *         and #Direction::N if \a l is equal to \a r.
   */
  static Dir dir(Key_param_type l, Bst_node const *r)
  { return dir(l, k(r)); }

  /// Is \a l greater than \a r.
  static bool greater(Key_param_type l, Key_param_type r)
  { return Compare()(r, l); }

  /// Is \a l greater than \a r.
  static bool greater(Key_param_type l, Bst_node const *r)
  { return greater(l, k(r)); }

  /// Is \a l greater than \a r.
  static bool greater(Bst_node const *l, Bst_node const *r)
  { return greater(k(l), k(r)); }
  /*@}*/

public:

  /**
   * \name Get default iterators for the ordered tree.
   */
  /*@{*/
  /**
   * \brief Get the constant forward iterator for the first element in the set.
   * \return Constant forward iterator for the first element in the set.
   */
  Const_iterator begin() const { return Const_iterator(head()); }
  /**
   * \brief Get the end marker for the constant forward iterator.
   * \return The end marker for the constant forward iterator.
   */
  Const_iterator end() const { return Const_iterator(); }

  /**
   * \brief Get the mutable forward iterator for the first element of the set.
   * \return The mutable forward iterator for the first element of the set.
   */
  Iterator begin() { return Iterator(head()); }
  /**
   * \brief Get the end marker for the mutable forward iterator.
   * \return The end marker for mutable forward iterator.
   */
  Iterator end() { return Iterator(); }

  /**
   * \brief Get the constant backward iterator for the last element in the set.
   * \return The constant backward iterator for the last element in the set.
   */
  Const_rev_iterator rbegin() const { return Const_rev_iterator(head()); }
  /**
   * \brief Get the end marker for the constant backward iterator.
   * \return The end marker for the constant backward iterator.
   */
  Const_rev_iterator rend() const { return Const_rev_iterator(); }

  /**
   * \brief Get the mutable backward iterator for the last element of the set.
   * \return The mutable backward iterator for the last element of the set.
   */
  Rev_iterator rbegin() { return Rev_iterator(head()); }
  /**
   * \brief Get the end marker for the mutable backward iterator.
   * \return The end marker for mutable backward iterator.
   */
  Rev_iterator rend() { return Rev_iterator(); }
  /*@}*/


  /**
   * \name Lookup functions.
   */
  //@{
  /**
   * \brief find the node with the given \a key.
   * \param key The key value of the element to search.
   * \return A pointer to the node with the given \a key, or
   *         NULL if \a key was not found.
   */
  Node *find_node(Key_param_type key) const;

  /**
   * \brief find the first node with a key not less than the given \a key.
   * \param key The key value of the element to search.
   * \return A pointer to the node with the given \a key, or
   *         NULL if \a key was not found.
   */
  Node *lower_bound_node(Key_param_type key) const;

  /**
   * \brief find the node with the given \a key.
   * \param key The key value of the element to search.
   * \return A valid iterator for the node with the given \a key,
   *         or an invalid iterator if \a key was not found.
   */
  Const_iterator find(Key_param_type key) const;

  //@}
};

/* find an element */
template< typename Node, typename Get_key, class Compare>
inline
Node *
Bst<Node, Get_key, Compare>::find_node(Key_param_type key) const
{
  Dir d;

  for (Bst_node *q = _head; q; q = Bst_node::next(q, d))
    {
      d = dir(key, q);
      if (d == Dir::N)
	return static_cast<Node*>(q);
    }
  return 0;
}

template< typename Node, typename Get_key, class Compare>
inline
Node *
Bst<Node, Get_key, Compare>::lower_bound_node(Key_param_type key) const
{
  Dir d;
  Bst_node *r = 0;

  for (Bst_node *q = _head; q; q = Bst_node::next(q, d))
    {
      d = dir(key, q);
      if (d == Dir::L)
	r = q; // found a node greater than key
      else if (d == Dir::N)
	return static_cast<Node*>(q);
    }
  return static_cast<Node*>(r);
}

/* find an element */
template< typename Node, typename Get_key, class Compare>
inline
typename Bst<Node, Get_key, Compare>::Const_iterator
Bst<Node, Get_key, Compare>::find(Key_param_type key) const
{
  Bst_node *q = _head;
  Bst_node *r = q;

  for (Dir d; q; q = Bst_node::next(q, d))
    {
      d = dir(key, q);
      if (d == Dir::N)
	return Iterator(static_cast<Node*>(q), static_cast<Node *>(r));

      if (d != Dir::L && q == r)
	r = Bst_node::next(q, d);
    }
  return Iterator();
}

}}
