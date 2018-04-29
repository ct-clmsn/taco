#include "taco/index_notation/index_notation.h"

#include <iostream>

#include "error/error_checks.h"
#include "taco/error/error_messages.h"
#include "taco/type.h"
#include "taco/format.h"

#include "taco/index_notation/schedule.h"
#include "taco/index_notation/index_notation_nodes.h"
#include "taco/index_notation/index_notation_rewriter.h"
#include "taco/index_notation/index_notation_printer.h"

#include "taco/util/name_generator.h"

using namespace std;

namespace taco {

// class IndexExpr
IndexExpr::IndexExpr(TensorVar var) : IndexExpr(new AccessNode(var,{})) {
}

IndexExpr::IndexExpr(long long val) : IndexExpr(new IntImmNode(val)) {
}

IndexExpr::IndexExpr(std::complex<double> val) : IndexExpr(new ComplexImmNode(val)) {
}

IndexExpr::IndexExpr(unsigned long long val) : IndexExpr(new UIntImmNode(val)) {
}

IndexExpr::IndexExpr(double val) : IndexExpr(new FloatImmNode(val)) {
}

void IndexExpr::splitOperator(IndexVar old, IndexVar left, IndexVar right) {
  const_cast<ExprNode*>(this->ptr)->splitOperator(old, left, right);
}
  
DataType IndexExpr::getDataType() const {
  return const_cast<ExprNode*>(this->ptr)->getDataType();
}

void IndexExpr::accept(IndexExprVisitorStrict *v) const {
  ptr->accept(v);
}

std::ostream& operator<<(std::ostream& os, const IndexExpr& expr) {
  if (!expr.defined()) return os << "IndexExpr()";
  IndexNotationPrinter printer(os);
  printer.print(expr);
  return os;
}

struct Equals : public IndexNotationVisitorStrict {
  bool eq = false;
  IndexExpr bExpr;
  IndexStmt bStmt;

  bool check(IndexExpr a, IndexExpr b) {
    this->bExpr = b;
    a.accept(this);
    return eq;
  }

  bool check(IndexStmt a, IndexStmt b) {
    this->bStmt = b;
    a.accept(this);
    return eq;
  }

  using IndexNotationVisitorStrict::visit;

  void visit(const AccessNode* anode) {
    if (!isa<AccessNode>(bExpr)) {
      eq = false;
      return;
    }

    auto bnode = to<AccessNode>(bExpr);
    if (anode->tensorVar != bnode->tensorVar) {
      eq = false;
      return;
    }
    if (anode->indexVars.size() != anode->indexVars.size()) {
      eq = false;
      return;
    }
    for (size_t i = 0; i < anode->indexVars.size(); i++) {
      if (anode->indexVars[i] != bnode->indexVars[i]) {
        eq = false;
        return;
      }
    }
    eq = true;
  }

  template <class T>
  bool unaryEquals(const T* anode, IndexExpr b) {
    if (!isa<T>(b)) {
      return false;
    }
    auto bnode = to<T>(b);
    if (!equals(anode->a, bnode->a)) {
      return false;
    }
    return true;
  }

  void visit(const NegNode* anode) {
    eq = unaryEquals(anode, bExpr);
  }

  void visit(const SqrtNode* anode) {
    eq = unaryEquals(anode, bExpr);
  }

  template <class T>
  bool binaryEquals(const T* anode, IndexExpr b) {
    if (!isa<T>(b)) {
      return false;
    }
    auto bnode = to<T>(b);
    if (!equals(anode->a, bnode->a) || !equals(anode->b, bnode->b)) {
      return false;
    }
    return true;
  }

  void visit(const AddNode* anode) {
    eq = binaryEquals(anode, bExpr);
  }

  void visit(const SubNode* anode) {
    eq = binaryEquals(anode, bExpr);
  }

  void visit(const MulNode* anode) {
    eq = binaryEquals(anode, bExpr);
  }

  void visit(const DivNode* anode) {
    eq = binaryEquals(anode, bExpr);
  }

  void visit(const ReductionNode* anode) {
    if (!isa<ReductionNode>(bExpr)) {
      eq = false;
      return;
    }
    auto bnode = to<ReductionNode>(bExpr);
    if (!(equals(anode->op, bnode->op) && equals(anode->a, bnode->a))) {
      eq = false;
      return;
    }
    eq = true;
  }

  template <class T>
  bool immediateEquals(const T* anode, IndexExpr bExpr) {
    if (!isa<T>(bExpr)) {
      return false;
    }
    auto bnode = to<T>(bExpr);
    if (anode->val != bnode->val) {
      return false;
    }
    return true;
  }

  void visit(const IntImmNode* anode) {
    eq = immediateEquals(anode, bExpr);
  }

  void visit(const FloatImmNode* anode) {
    eq = immediateEquals(anode, bExpr);
  }

  void visit(const ComplexImmNode* anode) {
    eq = immediateEquals(anode, bExpr);
  }

  void visit(const UIntImmNode* anode) {
    eq = immediateEquals(anode, bExpr);
  }

  void visit(const AssignmentNode* anode) {
    if (!isa<AssignmentNode>(bStmt.ptr)) {
      eq = false;
      return;
    }
    auto bnode = to<AssignmentNode>(bStmt.ptr);
    if (!equals(anode->lhs, bnode->lhs) || !equals(anode->rhs, bnode->rhs) ||
        !equals(anode->op, bnode->op)) {
      eq = false;
      return;
    }
    eq = true;
  }

  void visit(const ForallNode* anode) {
    taco_not_supported_yet;
  }

  void visit(const WhereNode* anode) {
    taco_not_supported_yet;
  }
};

bool equals(IndexExpr a, IndexExpr b) {
  if (!a.defined() && !b.defined()) {
    return true;
  }
  if ((a.defined() && !b.defined()) || (!a.defined() && b.defined())) {
    return false;
  }
  return Equals().check(a,b);
}

IndexExpr operator-(const IndexExpr& expr) {
  return new NegNode(expr.ptr);
}

IndexExpr operator+(const IndexExpr& lhs, const IndexExpr& rhs) {
  return new AddNode(lhs, rhs);
}

IndexExpr operator-(const IndexExpr& lhs, const IndexExpr& rhs) {
  return new SubNode(lhs, rhs);
}

IndexExpr operator*(const IndexExpr& lhs, const IndexExpr& rhs) {
  return new MulNode(lhs, rhs);
}

IndexExpr operator/(const IndexExpr& lhs, const IndexExpr& rhs) {
  return new DivNode(lhs, rhs);
}


// class Access
Access::Access(const AccessNode* n) : IndexExpr(n) {
}

Access::Access(const TensorVar& tensor, const std::vector<IndexVar>& indices)
    : Access(new AccessNode(tensor, indices)) {
}

const AccessNode* Access::getPtr() const {
  return static_cast<const AccessNode*>(ptr);
}

const TensorVar& Access::getTensorVar() const {
  return getPtr()->tensorVar;
}

const std::vector<IndexVar>& Access::getIndexVars() const {
  return getPtr()->indexVars;
}

Assignment Access::operator=(const IndexExpr& expr) {
  TensorVar result = getTensorVar();
  Assignment assignment = Assignment(result, getIndexVars(), expr);
  const_cast<AccessNode*>(getPtr())->setAssignment(assignment);
  return assignment;
}

Assignment Access::operator=(const Access& expr) {
  return operator=(static_cast<IndexExpr>(expr));
}

Assignment Access::operator=(const TensorVar& var) {
  return operator=(Access(var));
}

Assignment Access::operator+=(const IndexExpr& expr) {
  TensorVar result = getTensorVar();
  Assignment assignment = Assignment(result, getIndexVars(), expr, new AddNode);
  const_cast<AccessNode*>(getPtr())->setAssignment(assignment);
  return assignment;
}


// class Sum
Reduction::Reduction(const ReductionNode* n) : IndexExpr(n) {
}

Reduction::Reduction(IndexExpr op, IndexVar var, IndexExpr expr)
    : Reduction(new ReductionNode(op, var, expr)) {

}


// class TensorExpr
IndexStmt::IndexStmt() : util::IntrusivePtr<const IndexStmtNode>(nullptr) {
}

IndexStmt::IndexStmt(const IndexStmtNode* n)
    : util::IntrusivePtr<const IndexStmtNode>(n) {
}

void IndexStmt::accept(IndexNotationVisitorStrict *v) const {
  ptr->accept(v);
}

bool equals(IndexStmt a, IndexStmt b) {
  if (!a.defined() && !b.defined()) {
    return true;
  }
  if ((a.defined() && !b.defined()) || (!a.defined() && b.defined())) {
    return false;
  }
  return Equals().check(a,b);
}

std::ostream& operator<<(std::ostream& os, const IndexStmt& expr) {
  if (!expr.defined()) return os << "TensorExpr()";
  IndexNotationPrinter printer(os);
  printer.print(expr);
  return os;
}

Reduction sum(IndexVar i, IndexExpr expr) {
  return Reduction(new AddNode, i, expr);
}


// class Assignment
Assignment::Assignment(const AssignmentNode* n) : IndexStmt(n) {
}

Assignment::Assignment(Access lhs, IndexExpr rhs, IndexExpr op)
    : Assignment(new AssignmentNode(lhs, rhs, op)) {
}

Assignment::Assignment(TensorVar tensor, vector<IndexVar> indices,
                       IndexExpr rhs, IndexExpr op)
    : Assignment(Access(tensor, indices), rhs, op) {
}

Access Assignment::getLhs() const {
  return getNode(*this)->lhs;
}

IndexExpr Assignment::getRhs() const {
  return getNode(*this)->rhs;
}

IndexExpr Assignment::getOp() const {
  return getNode(*this)->op;
}

const std::vector<IndexVar>& Assignment::getFreeVars() const {
  return getLhs().getIndexVars();
}

template <> bool isa<Assignment>(IndexStmt s) {
  return isa<AssignmentNode>(s.ptr);
}

template <> Assignment to<Assignment>(IndexStmt s) {
  taco_iassert(isa<Assignment>(s));
  return Assignment(to<AssignmentNode>(s.ptr));
}


// class Forall
Forall::Forall(const ForallNode* n) : IndexStmt(n) {
}

Forall::Forall(IndexVar indexVar, IndexStmt stmt)
    : Forall(new ForallNode(indexVar, stmt)) {
}

IndexVar Forall::getIndexVar() const {
  return getNode(*this)->indexVar;
}

IndexStmt Forall::getStmt() const {
  return getNode(*this)->stmt;
}

Forall forall(IndexVar i, IndexStmt expr) {
  return Forall(i, expr);
}

template <> bool isa<Forall>(IndexStmt s) {
  return isa<ForallNode>(s.ptr);
}

template <> Forall to<Forall>(IndexStmt s) {
  taco_iassert(isa<Forall>(s));
  return Forall(to<ForallNode>(s.ptr));
}


// class Where
Where::Where(const WhereNode* n) : IndexStmt(n) {
}

Where::Where(IndexStmt consumer, IndexStmt producer)
    : Where(new WhereNode(consumer, producer)) {
}

IndexStmt Where::getConsumer() {
  return getNode(*this)->consumer;
}

IndexStmt Where::getProducer() {
  return getNode(*this)->consumer;
}

Where where(IndexStmt consumer, IndexStmt producer) {
  return Where(consumer, producer);
}

template <> bool isa<Where>(IndexStmt s) {
  return isa<WhereNode>(s.ptr);
}

template <> Where to<Where>(IndexStmt s) {
  taco_iassert(isa<Where>(s));
  return Where(to<WhereNode>(s.ptr));
}


// class IndexVar
struct IndexVar::Content {
  string name;
};

IndexVar::IndexVar() : IndexVar(util::uniqueName('i')) {}

IndexVar::IndexVar(const std::string& name) : content(new Content) {
  content->name = name;
}

std::string IndexVar::getName() const {
  return content->name;
}

bool operator==(const IndexVar& a, const IndexVar& b) {
  return a.content == b.content;
}

bool operator<(const IndexVar& a, const IndexVar& b) {
  return a.content < b.content;
}

std::ostream& operator<<(std::ostream& os, const IndexVar& var) {
  return os << var.getName();
}


// class TensorVar
struct TensorVar::Content {
  string name;
  Type type;
  Format format;

  /// An expression to compute this tensor variable.
  Assignment assignment;

  Schedule schedule;
};

TensorVar::TensorVar() : TensorVar(Type()) {
}

TensorVar::TensorVar(const Type& type) : TensorVar(type, Dense) {
}

TensorVar::TensorVar(const std::string& name, const Type& type)
    : TensorVar(name, type, Dense) {
}

TensorVar::TensorVar(const Type& type, const Format& format)
    : TensorVar(util::uniqueName('A'), type, format) {
}

TensorVar::TensorVar(const string& name, const Type& type, const Format& format)
    : content(new Content) {
  content->name = name;
  content->type = type;
  content->format = format;
}

std::string TensorVar::getName() const {
  return content->name;
}

int TensorVar::getOrder() const {
  return content->type.getShape().getOrder();
}

const Type& TensorVar::getType() const {
  return content->type;
}

const Format& TensorVar::getFormat() const {
  return content->format;
}

const Assignment& TensorVar::getAssignment() const {
  return content->assignment;
}

const Schedule& TensorVar::getSchedule() const {
  struct GetSchedule : public IndexNotationVisitor {
    using IndexNotationVisitor::visit;
    Schedule schedule;
    void visit(const BinaryExprNode* expr) {
      for (auto& operatorSplit : expr->getOperatorSplits()) {
        schedule.addOperatorSplit(operatorSplit);
      }
    }
  };
  GetSchedule getSchedule;
  content->schedule.clearOperatorSplits();
  getSchedule.schedule = content->schedule;
  getAssignment().getRhs().accept(&getSchedule);
  return content->schedule;
}

void TensorVar::setName(std::string name) {
  content->name = name;
}

void TensorVar::setAssignment(Assignment assignment) {
  auto freeVars = assignment.getLhs().getIndexVars();
  auto indexExpr = assignment.getRhs();

  auto shape = getType().getShape();
  taco_uassert(error::dimensionsTypecheck(freeVars, indexExpr, shape))
      << error::expr_dimension_mismatch << " "
      << error::dimensionTypecheckErrors(freeVars, indexExpr, shape);

  // The following are index expressions the implementation doesn't currently
  // support, but that are planned for the future.
  taco_uassert(!error::containsTranspose(this->getFormat(), freeVars, indexExpr))
      << error::expr_transposition;

  content->assignment = assignment;
}

const Access TensorVar::operator()(const std::vector<IndexVar>& indices) const {
  taco_uassert((int)indices.size() == getOrder()) <<
      "A tensor of order " << getOrder() << " must be indexed with " <<
      getOrder() << " variables, but is indexed with:  " << util::join(indices);
  return Access(new AccessNode(*this, indices));
}

Access TensorVar::operator()(const std::vector<IndexVar>& indices) {
  taco_uassert((int)indices.size() == getOrder()) <<
      "A tensor of order " << getOrder() << " must be indexed with " <<
      getOrder() << " variables, but is indexed with:  " << util::join(indices);
  return Access(new AccessNode(*this, indices));
}

Assignment TensorVar::operator=(const IndexExpr& expr) {
  taco_uassert(getOrder() == 0)
      << "Must use index variable on the left-hand-side when assigning an "
      << "expression to a non-scalar tensor.";
  Assignment assignment = Assignment(*this, {}, expr);
  setAssignment(assignment);
  return assignment;
}

Assignment TensorVar::operator+=(const IndexExpr& expr) {
  taco_uassert(getOrder() == 0)
      << "Must use index variable on the left-hand-side when assigning an "
      << "expression to a non-scalar tensor.";
  Assignment assignment = Assignment(*this, {}, expr, new AddNode);
  setAssignment(assignment);
  return assignment;
}

bool operator==(const TensorVar& a, const TensorVar& b) {
  return a.content == b.content;
}

bool operator<(const TensorVar& a, const TensorVar& b) {
  return a.content < b.content;
}

std::ostream& operator<<(std::ostream& os, const TensorVar& var) {
  return os << var.getName() << " : " << var.getType();
}


// functions
vector<IndexVar> getIndexVars(const IndexExpr& expr) {
  vector<IndexVar> indexVars;
  set<IndexVar> seen;
  match(expr,
    function<void(const AccessNode*)>([&](const AccessNode* op) {
      for (auto& var : op->indexVars) {
        if (!util::contains(seen, var)) {
          seen.insert(var);
          indexVars.push_back(var);
        }
      }
    })
  );
  return indexVars;
}

set<IndexVar> getIndexVars(const TensorVar& tensor) {
  Assignment assignment = tensor.getAssignment();
  auto indexVars = util::combine(assignment.getLhs().getIndexVars(),
                                 getIndexVars(assignment.getRhs()));
  return set<IndexVar>(indexVars.begin(), indexVars.end());
}

map<IndexVar,Dimension> getIndexVarRanges(const TensorVar& tensor) {
  map<IndexVar, Dimension> indexVarRanges;

  auto& freeVars = tensor.getAssignment().getFreeVars();
  auto& type = tensor.getType();
  for (size_t i = 0; i < freeVars.size(); i++) {
    indexVarRanges.insert({freeVars[i], type.getShape().getDimension(i)});
  }

  match(tensor.getAssignment().getRhs(),
    function<void(const AccessNode*)>([&indexVarRanges](const AccessNode* op) {
      auto& tensor = op->tensorVar;
      auto& vars = op->indexVars;
      auto& type = tensor.getType();
      for (size_t i = 0; i < vars.size(); i++) {
        indexVarRanges.insert({vars[i], type.getShape().getDimension(i)});
      }
    })
  );
  
  return indexVarRanges;
}

struct Simplify : public ExprRewriterStrict {
public:
  Simplify(const set<Access>& zeroed) : zeroed(zeroed) {}

private:
  using ExprRewriterStrict::visit;

  set<Access> zeroed;
  void visit(const AccessNode* op) {
    if (util::contains(zeroed, op)) {
      expr = IndexExpr();
    }
    else {
      expr = op;
    }
  }

  template <class T>
  IndexExpr visitUnaryOp(const T *op) {
    IndexExpr a = rewrite(op->a);
    if (!a.defined()) {
      return IndexExpr();
    }
    else if (a == op->a) {
      return op;
    }
    else {
      return new T(a);
    }
  }

  void visit(const NegNode* op) {
    expr = visitUnaryOp(op);
  }

  void visit(const SqrtNode* op) {
    expr = visitUnaryOp(op);
  }

  template <class T>
  IndexExpr visitDisjunctionOp(const T *op) {
    IndexExpr a = rewrite(op->a);
    IndexExpr b = rewrite(op->b);
    if (!a.defined() && !b.defined()) {
      return IndexExpr();
    }
    else if (!a.defined()) {
      return b;
    }
    else if (!b.defined()) {
      return a;
    }
    else if (a == op->a && b == op->b) {
      return op;
    }
    else {
      return new T(a, b);
    }
  }

  template <class T>
  IndexExpr visitConjunctionOp(const T *op) {
    IndexExpr a = rewrite(op->a);
    IndexExpr b = rewrite(op->b);
    if (!a.defined() || !b.defined()) {
      return IndexExpr();
    }
    else if (a == op->a && b == op->b) {
      return op;
    }
    else {
      return new T(a, b);
    }
  }

  void visit(const AddNode* op) {
    expr = visitDisjunctionOp(op);
  }

  void visit(const SubNode* op) {
    expr = visitDisjunctionOp(op);
  }

  void visit(const MulNode* op) {
    expr = visitConjunctionOp(op);
  }

  void visit(const DivNode* op) {
    expr = visitConjunctionOp(op);
  }

  void visit(const ReductionNode* op) {
    IndexExpr a = rewrite(op->a);
    if (!a.defined()) {
      expr = IndexExpr();
    }
    else if (a == op->a) {
      expr = op;
    }
    else {
      expr = new ReductionNode(op->op, op->var, a);
    }
  }

  void visit(const IntImmNode* op) {
    expr = op;
  }

  void visit(const FloatImmNode* op) {
    expr = op;
  }

  void visit(const UIntImmNode* op) {
    expr = op;
  }

  void visit(const ComplexImmNode* op) {
    expr = op;
  }
};

IndexExpr simplify(const IndexExpr& expr, const set<Access>& zeroed) {
  return Simplify(zeroed).rewrite(expr);
}

set<IndexVar> getVarsWithoutReduction(const IndexExpr& expr) {
  struct GetVarsWithoutReduction : public IndexNotationVisitor {
    set<IndexVar> indexvars;

    set<IndexVar> get(const IndexExpr& expr) {
      indexvars.clear();
      expr.accept(this);
      return indexvars;
    }

    using IndexExprVisitorStrict::visit;

    void visit(const AccessNode* op) {
      indexvars.insert(op->indexVars.begin(), op->indexVars.end());
    }

    void visit(const ReductionNode* op) {
      indexvars.erase(op->var);
    }
  };
  return GetVarsWithoutReduction().get(expr);
}

bool verify(const Assignment& assignment) {
 IndexExpr expr = assignment.getRhs();
 std::vector<IndexVar> free = assignment.getLhs().getIndexVars();
  set<IndexVar> freeVars(free.begin(), free.end());
  for (auto& var : getVarsWithoutReduction(expr)) {
    if (!util::contains(freeVars, var)) {
      return false;
    }
  }
  return true;
}

bool verify(const TensorVar& var) {
  return verify(var.getAssignment());
}

bool isEinsumNotation(IndexExpr expr) {
  struct VerifyEinsum : public IndexNotationVisitor {
    bool isEinsum;
    bool mulnodeVisited;

    bool verify(IndexExpr expr) {
      // Einsum until proved otherwise
      isEinsum = true;

      // Additions are not allowed under the first multplication
      mulnodeVisited = false;

      expr.accept(this);
      return isEinsum;
    }

    using IndexNotationVisitor::visit;

    void visit(const AddNode* node) {
      if (mulnodeVisited) {
        isEinsum = false;
        return;
      }
      node->a.accept(this);
      node->b.accept(this);
    }

    void visit(const SubNode* node) {
      if (mulnodeVisited) {
        isEinsum = false;
        return;
      }
      node->a.accept(this);
      node->b.accept(this);
    }

    void visit(const MulNode* node) {
      bool topMulNode = !mulnodeVisited;
      mulnodeVisited = true;
      node->a.accept(this);
      node->b.accept(this);
      if (topMulNode) {
        mulnodeVisited = false;
      }
    }

    void visit(const BinaryExprNode* node) {
      isEinsum = false;
    }

    void visit(const ReductionNode* node) {
      isEinsum = false;
    }
  };
  return VerifyEinsum().verify(expr);
}

Assignment einsum(const Assignment& assignment) {
  IndexExpr expr = assignment.getRhs();
  std::vector<IndexVar> free = assignment.getLhs().getIndexVars();
  if (!isEinsumNotation(expr)) {
    return assignment;
  }

  struct Einsum : IndexNotationRewriter {
    Einsum(const std::vector<IndexVar>& free) : free(free.begin(), free.end()){}

    std::set<IndexVar> free;
    bool onlyOneTerm;

    IndexExpr addReductions(IndexExpr expr) {
      auto vars = getIndexVars(expr);
      for (auto& var : util::reverse(vars)) {
        if (!util::contains(free, var)) {
          expr = sum(var,expr);
        }
      }
      return expr;
    }

    IndexExpr einsum(const IndexExpr& expr) {
      onlyOneTerm = true;
      IndexExpr einsumexpr = rewrite(expr);

      if (onlyOneTerm) {
        einsumexpr = addReductions(einsumexpr);
      }

      return einsumexpr;
    }

    using IndexNotationRewriter::visit;

    void visit(const AddNode* op) {
      // Sum every reduction variables over each term
      onlyOneTerm = false;

      IndexExpr a = addReductions(op->a);
      IndexExpr b = addReductions(op->b);
      if (a == op->a && b == op->b) {
        expr = op;
      }
      else {
        expr = new AddNode(a, b);
      }
    }

    void visit(const SubNode* op) {
      // Sum every reduction variables over each term
      onlyOneTerm = false;

      IndexExpr a = addReductions(op->a);
      IndexExpr b = addReductions(op->b);
      if (a == op->a && b == op->b) {
        expr = op;
      }
      else {
        expr = new SubNode(a, b);
      }
    }
  };
  return Assignment(assignment.getLhs(), Einsum(free).einsum(expr),
                    assignment.getOp());
}

}