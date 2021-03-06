/// \file
/// \brief Defines class JTree, which implements the junction tree algorithm

#ifndef PDBNTK_JTREE_H_ 
#define PDBNTK_JTREE_H_ 

#include <dai/bp.h>
#include <dai/cbp.h>

#include "dai/weightedgraph.h"
#include "daialg.h"
#include "graph/cluster_graph.h"
#include "graph/factor_graph.h"

#include <vector>
#include <string>

namespace pdbntk {

/// Exact inference algorithm using junction tree
/** The junction tree algorithm uses message passing on a junction tree to calculate
 *  exact marginal probability distributions ("beliefs") for specified cliques
 *  (outer regions) and separators (intersections of pairs of cliques).
 *
 *  There are two variants, the sum-product algorithm (corresponding to 
 *  finite temperature) and the max-product algorithm (corresponding to 
 *  zero temperature).
 */
class JTree : public DAIAlgRG {
 public:
      /// Parameters for JTree
   struct Properties {
     /// Enumeration of possible JTree updates
     /** There are two types of updates:
      *  - HUGIN similar to those in HUGIN
      *  - SHSH Shafer-Shenoy type
      */
     DAI_ENUM(UpdateType, HUGIN, SHSH);

     /// Enumeration of inference variants
     /** There are two inference variants:
      *  - SUMPROD Sum-Product
      *  - MAXPROD Max-Product (equivalent to Min-Sum)
      */
     DAI_ENUM(InfType,SUMPROD,MAXPROD);

     /// Enumeration of elimination cost functions used for constructing the junction tree
     /** The cost of eliminating a variable can be (\see [\ref KoF09], page 314)):
      *  - MINNEIGHBORS the number of neighbors it has in the current adjacency graph;
      *  - MINWEIGHT the product of the number of states of all neighbors in the current adjacency graph;
      *  - MINFILL the number of edges that need to be added to the adjacency graph due to the elimination;
      *  - WEIGHTEDMINFILL the sum of weights of the edges that need to be added to the adjacency graph
      *    due to the elimination, where a weight of an edge is the produt of weights of its constituent
      *    vertices.
      *  The elimination sequence is chosen greedily in order to minimize the cost.
      */
     DAI_ENUM(HeuristicType,MINNEIGHBORS,MINWEIGHT,MINFILL,WEIGHTEDMINFILL);

     /// Type of updates
     UpdateType updates;

     /// Type of inference
     InfType inference;

     /// Heuristic to use for constructing the junction tree
     HeuristicType heuristic;

     /// Maximum memory to use in bytes (0 means unlimited)
     size_t maxmem;

     NodeSet root;
   };

 public:
   /// \name Constructors/destructors
   //@{
   /// Default constructor
   JTree() : DAIAlgRG(), RTree(), Qa(), Qb(), mes_(), logz_(), props_() {}

   /// Construct from FactorGraph \a fg and PropertySet \a opts
   /** \param fg factor graph
    ** \param opts Parameters @see Properties
    *  \param automatic if \c true, construct the junction tree automatically,
    *  using the heuristic in opts['heuristic'].
    */
   JTree(const FactorGraph &fg, 
       const dai::PropertySet &opts = dai::PropertySet(),
       bool automatic = true);
   //@}


   /// \name General InfAlg interface
   //@{
   virtual JTree* clone() const { return new JTree(*this); }
   virtual JTree* construct(const FactorGraph &fg, const dai::PropertySet &opts ) const { return new JTree( fg, opts ); }
   virtual std::string name() const { return "JTREE"; }
   virtual Factor belief( const NodeSet &vs ) const;
   virtual std::vector<Factor> beliefs() const;
   virtual Real logZ() const;
   /** \pre Assumes that run() has been called and that \a Props.inference == \c MAXPROD
   */
   std::vector<Node*> findMaximum() const;
   virtual void init() {}
   virtual void init( const NodeSet &/*ns*/ ) {}
   virtual Real run();
   virtual Real maxDiff() const { return 0.0; }
   virtual size_t Iterations() const { return 1UL; }
   virtual void setProperties( const dai::PropertySet &opts );
   virtual dai::PropertySet getProperties() const;
   virtual std::string printProperties() const;
   //@}


   /// \name Additional interface specific for JTree
   //@{
   /// Constructs a junction tree based on the cliques \a cl (corresponding to some elimination sequence).
   /** First, constructs a weighted graph, where the nodes are the elements of \a cl, and 
    *  each edge is weighted with the cardinality of the intersection of the state spaces of the nodes. 
    *  Then, a maximal spanning tree for this weighted graph is calculated.
    *  Subsequently, a corresponding region graph is built:
    *    - the outer regions correspond with the cliques and have counting number 1;
    *    - the inner regions correspond with the seperators, i.e., the intersections of two 
    *      cliques that are neighbors in the spanning tree, and have counting number -1
    *      (except empty ones, which have counting number 0);
    *    - inner and outer regions are connected by an edge if the inner region is a
    *      seperator for the outer region.
    *  Finally, Beliefs are constructed.
    *  If \a verify == \c true, checks whether each factor is subsumed by a clique.
    */
   void construct( const FactorGraph &fg, const std::vector<NodeSet> &cl, bool verify=false );

   /// Constructs a junction tree based on the cliques \a cl (corresponding to some elimination sequence).
   /** Invokes construct() and then constructs messages.
    *  \see construct()
    */
   void GenerateJT( const FactorGraph &fg, const std::vector<NodeSet> &cl );

   /// Returns constant reference to the message from outer region \a alpha to its \a _beta 'th neighboring inner region
   const Factor & message( size_t alpha, size_t _beta ) const { return mes_[alpha][_beta]; }
   /// Returns reference to the message from outer region \a alpha to its \a _beta 'th neighboring inner region
   Factor & message(size_t alpha, size_t _beta) { return mes_[alpha][_beta]; }

   /// Runs junction tree algorithm using HUGIN (message-free) updates
   /** \note The initial messages may be arbitrary; actually they are not used at all.
   */
   void runHUGIN();

   /// Runs junction tree algorithm using Shafer-Shenoy updates
   /** \note The initial messages may be arbitrary.
   */
   void runShaferShenoy();

   /// Finds an efficient subtree for calculating the marginal of the variables in \a vs
   /** First, the current junction tree is reordered such that it gets as root the clique 
    *  that has maximal state space overlap with \a vs. Then, the minimal subtree
    *  (starting from the root) is identified that contains all the variables in \a vs
    *  and also the outer region with index \a PreviousRoot (if specified). Finally,
    *  the current junction tree is reordered such that this minimal subtree comes
    *  before the other edges, and the size of the minimal subtree is returned.
    */
   size_t findEfficientTree(const NodeSet& vs, dai::RootedTree &Tree, size_t PreviousRoot=(size_t)-1 ) const;

   /// Calculates the marginal of a set of variables (using cutset conditioning, if necessary)
   /** \pre assumes that run() has been called already
   */
   Factor calcMarginal( const NodeSet& vs );
   //@}

  /// The junction tree (stored as a rooted tree)
   dai::RootedTree RTree;

   /// Outer region beliefs
   std::vector<Factor> Qa;

   /// Inner region beliefs
   std::vector<Factor> Qb;

  private:
   size_t FindRoot(const std::vector<NodeSet> &cl) const;

   /// Stores the messages
   std::vector<std::vector<Factor> >  mes_;

   /// Stores the logarithm of the partition sum
   Real logz_;

   Properties props_;
};

/// Calculates upper bound to the treewidth of a FactorGraph, using the specified heuristic
/** \relates JTree
 *  \param fg the factor graph for which the treewidth should be bounded
 *  \param fn the heuristic cost function used for greedy variable elimination
 *  \param maxStates maximum total number of states in outer regions of junction tree (0 means no limit)
 *  \throws OUT_OF_MEMORY if the total number of states becomes larger than maxStates
 *  \return a pair (number of variables in largest clique, number of states in largest clique)
 */
std::pair<size_t, dai::BigInt> boundTreewidth(const FactorGraph &fg, greedyVariableElimination::eliminationCostFunction fn, size_t maxStates=0 );

}
#endif // PDBNTK_JTREE_H_
