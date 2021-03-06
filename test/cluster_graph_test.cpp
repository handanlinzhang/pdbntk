#include "graph/cluster_graph.h"
#include "cpd/cpd_factory.h"
#include "gtest/gtest.h"
#include "glog/logging.h"

#include <memory>
#include <vector>
#include <ostream>

using pdbntk::Node;
using pdbntk::NodeSet;
using pdbntk::Factor;
using pdbntk::CondProbDist;
using pdbntk::CPDFactory;
using std::shared_ptr;
using std::vector;
using std::unique_ptr;
using pdbntk::eliminationCost_WeightedMinFill;
using pdbntk::ClusterGraph;

class ClusterGraphTestF : public testing::Test {
 protected:
  virtual void SetUp() {
    vector<Factor> factors;
    cpd1_.reset(CPDFactory::NewDiscreteCPD(13));
    cpd2_.reset(CPDFactory::NewDiscreteCPD(44));
    cpd3_.reset(CPDFactory::NewDiscreteCPD(2));
    cpd4_.reset(CPDFactory::NewGaussianCPD(9));
    cpd5_.reset(CPDFactory::NewDiscreteCPD(13));
    cpd6_.reset(CPDFactory::NewDiscreteCPD(44));

    uint di = 0;
    n1_.reset(new Node(1, di, cpd1_.get(), true));
    di += cpd1_->NodeSize();
    n2_.reset(new Node(2, di, cpd2_.get()));
    di += cpd2_->NodeSize();
    n3_.reset(new Node(3, di, cpd3_.get(), true));
    di += cpd3_->NodeSize();
    n4_.reset(new Node(4, di, cpd4_.get(), true));
    di += cpd4_->NodeSize();
    n5_.reset(new Node(5, di, cpd5_.get(), true));
    di += cpd5_->NodeSize();
    n6_.reset(new Node(6, di, cpd6_.get()));
    di += cpd6_->NodeSize();
    n7_.reset(new Node(7, di, cpd3_.get(), true));
    di += cpd3_->NodeSize();
    n8_.reset(new Node(8, di, cpd4_.get(), true));

    NodeSet ns1(n1_.get(), n2_.get());
    factors.push_back(Factor(ns1 | n3_.get()));

    NodeSet ns2(n2_.get(), n5_.get());
    factors.push_back(Factor(ns2 | n6_.get()));

    factors.push_back(Factor(NodeSet(n6_.get(), n8_.get())));

    NodeSet ns4(n5_.get(), n6_.get());
    factors.push_back(Factor(ns4 | n7_.get()));

    NodeSet ns5(n1_.get(), n3_.get());
    factors.push_back(Factor(ns5 | n5_.get()));

    pdbntk::FactorGraph fg(factors);
    cg_.reset(new ClusterGraph(fg, true));
  } 
  
  unique_ptr<ClusterGraph> cg_;
  unique_ptr<CondProbDist> cpd1_, cpd2_, cpd3_, cpd4_, cpd5_, cpd6_;
  unique_ptr<Node> n1_, n2_, n3_, n4_, n5_, n6_, n7_, n8_;
};

TEST_F(ClusterGraphTestF, ClusterGraph) {
  EXPECT_EQ((uint) 7, cg_->nrNodes());
  EXPECT_EQ((uint) 5, cg_->nrClusters());
}

TEST_F(ClusterGraphTestF, EleiminationOrder) {
  EXPECT_EQ((size_t) 0, eliminationCost_WeightedMinFill(*cg_.get(), 0));
  EXPECT_EQ((size_t) 88, eliminationCost_WeightedMinFill(*cg_.get(), 1));
}

TEST(ClusterGraphTest, MaximalCliques) {
  vector<Factor> factors;

  unique_ptr<CondProbDist> cpd1(CPDFactory::NewDiscreteCPD(13));
  unique_ptr<CondProbDist> cpd2(CPDFactory::NewDiscreteCPD(44));
  unique_ptr<CondProbDist> cpd3(CPDFactory::NewDiscreteCPD(2));
  unique_ptr<CondProbDist> cpd4(CPDFactory::NewGaussianCPD(9));
  unique_ptr<CondProbDist> cpd5(CPDFactory::NewDiscreteCPD(13));
  unique_ptr<CondProbDist> cpd6(CPDFactory::NewDiscreteCPD(44));

  uint di = 0;
  Node n1(1, di, cpd1.get(), true);
  di += cpd1->NodeSize();
  Node n2(2, di, cpd2.get());
  di += cpd2->NodeSize();
  Node n3(3, di, cpd3.get(), true);
  di += cpd3->NodeSize();
  Node n4(4, di, cpd4.get(), true);
  di += cpd4->NodeSize();
  Node n5(5, di, cpd5.get(), true);
  di += cpd5->NodeSize();
  Node n6(6, di, cpd6.get());
  di += cpd6->NodeSize();
  Node n7(7, di, cpd3.get(), true);
  di += cpd3->NodeSize();
  Node n8(8, di, cpd4.get(), true);

  NodeSet ns1(&n1, &n2);
  factors.push_back(Factor(ns1 | &n3));

  NodeSet ns2(&n2, &n5);
  factors.push_back(Factor(ns2 | &n6));

  factors.push_back(Factor(NodeSet(&n6, &n8)));

  NodeSet ns4(&n5, &n6);
  factors.push_back(Factor(ns4 | &n7));

  NodeSet ns5(&n1, &n3);
  factors.push_back(Factor(ns5 | &n5));

  factors.push_back(Factor(NodeSet(&n1, &n2)));

  pdbntk::FactorGraph fg(factors);
  pdbntk::ClusterGraph cg(fg, true);

  EXPECT_EQ((uint) 7, cg.nrNodes());
  EXPECT_EQ((uint) 5, cg.nrClusters());

  pdbntk::ClusterGraph cg1(fg, false);
  EXPECT_EQ((uint) 6, cg1.nrClusters());
}
