#include "statement.h"


using namespace std;

namespace ast {

	using runtime::Closure;
	using runtime::Context;
	using runtime::ObjectHolder;

	namespace {
		const string ADD_METHOD = "__add__"s;
		const string INIT_METHOD = "__init__"s;
		const string NONE_OBJECT = "None"s;
	}  // namespace

	ObjectHolder Assignment::Execute(Closure& closure, Context& context) {
		ObjectHolder newVar = rv_->Execute(closure, context);
		return closure[var_name_] = newVar;
	}

	Assignment::Assignment(std::string var, std::unique_ptr<Statement> rv)
		: var_name_(move(var))
		, rv_(move(rv))
	{
	}

	VariableValue::VariableValue(const std::string& var_name)
		: var_name_({ var_name })
	{
	}

	VariableValue::VariableValue(std::vector<std::string> dotted_ids)
	{
		// Забираем первое значение из очереди в оборот
		if (dotted_ids.size() > 0) {
			var_name_ = move(dotted_ids[0]);
			for (size_t i = 1; i < dotted_ids.size(); ++i) {
				dotted_ids_.push_back(move(dotted_ids[i]));
			}
		}
	}

	ObjectHolder VariableValue::Execute(Closure& closure, Context& context) {
		auto it = closure.find(var_name_);
		if (it != closure.end()) {
			ObjectHolder& result = it->second;

			if (dotted_ids_.size() > 0) {
				// рекурсивно вычисляем значения всех переменных
				runtime::ClassInstance* obj = result.TryAs<runtime::ClassInstance>();
				if (obj) {
					return VariableValue(dotted_ids_).Execute(obj->Fields(), context);
				}
				else {
					throw std::runtime_error("Variable " + var_name_ + " is not class"s);
				}
			}

			return result;
		}
		else {
			throw std::runtime_error("Variable "s + var_name_ + " not found"s);
		}
	}

	unique_ptr<Print> Print::Variable(const std::string& name) {
		return make_unique<Print>(make_unique<VariableValue>(name));
	}

	Print::Print(unique_ptr<Statement> argument)
	{
		args_.push_back(move(argument));
	}

	Print::Print(vector<unique_ptr<Statement>> args)
		: args_(move(args))
	{
	}

	ObjectHolder Print::Execute(Closure& closure, Context& context) {
		bool first = true;
		auto& os = context.GetOutputStream();

		for (auto& arg : args_) {
			auto result = arg->Execute(closure, context);

			os << (first ? "" : " ");
			if (result) {
				result->Print(os, context);
			}
			else {
				os << NONE_OBJECT;
			}

			first = false;
		}
		os << "\n"s;
		return {};
	}

	MethodCall::MethodCall(std::unique_ptr<Statement> object, std::string method,
		std::vector<std::unique_ptr<Statement>> args)
		: object_(move(object))
		, method_(move(method))
		, args_(move(args))
	{
	}

	ObjectHolder MethodCall::Execute(Closure& closure, Context& context) {
		runtime::ClassInstance* clsInst = object_->Execute(closure, context).TryAs<runtime::ClassInstance>();

		if (clsInst) {
			if (clsInst->HasMethod(method_, args_.size())) {
				std::vector<runtime::ObjectHolder> actual_args;
				for (auto& arg : args_) {
					actual_args.emplace_back(arg->Execute(closure, context));
				}

				return clsInst->Call(method_, actual_args, context);
			}
			throw runtime_error("Class has no method "s + method_);
		}

		throw runtime_error("Object is not class instance"s);
	}

	ObjectHolder Stringify::Execute(Closure& closure, Context& context) {
		ObjectHolder obj = arg_->Execute(closure, context);
		if (obj) {
			ostringstream os;
			obj->Print(os, context);
			return ObjectHolder::Own(runtime::String(os.str()));
		}

		return ObjectHolder::Own(runtime::String(NONE_OBJECT));
	}

	ObjectHolder Add::Execute(Closure& closure, Context& context) {
		ObjectHolder lhs = lhs_->Execute(closure, context);
		ObjectHolder rhs = rhs_->Execute(closure, context);

		//  объект1 + объект2, если у объект1 - пользовательский класс с методом _add__(rhs)
		runtime::ClassInstance* lhs_is_class = lhs.TryAs<runtime::ClassInstance>();
		if (lhs_is_class) {
			return lhs_is_class->Call(ADD_METHOD, { rhs }, context);
		}

		{
			auto l = lhs.TryAs<runtime::Number>(); auto r = rhs.TryAs<runtime::Number>();
			if (l && r) {
				return ObjectHolder::Own<runtime::Number>(l->GetValue() + r->GetValue());
			}
		}
		{
			auto l = lhs.TryAs<runtime::String>(); auto r = rhs.TryAs<runtime::String>();
			if (l && r) {
				return ObjectHolder::Own<runtime::String>(l->GetValue() + r->GetValue());
			}
		}

		throw std::runtime_error("Cannot execute binary operation"s);
	}

	ObjectHolder Sub::Execute(Closure& closure, Context& context) {

		return ObjectHolder::Own<runtime::Number>(
			NumberBynaryOperation(
				lhs_, rhs_, closure, context,
				[](const int a, const int b) {
					return a - b;
				}
		));
	}

	ObjectHolder Mult::Execute(Closure& closure, Context& context) {
		return ObjectHolder::Own<runtime::Number>(
			NumberBynaryOperation(
				lhs_, rhs_, closure, context,
				[](const int a, const int b) {
					return a * b;
				}
		));
	}

	ObjectHolder Div::Execute(Closure& closure, Context& context) {

		return ObjectHolder::Own<runtime::Number>(
			NumberBynaryOperation(
				lhs_, rhs_, closure, context,
				[](const int a, const int b) {
					return a / b;
				}
		));
	}

	ObjectHolder Compound::Execute(Closure& closure, Context& context) {
		for (auto& arg : args_) {
			arg->Execute(closure, context);
		}
		return ObjectHolder::None();
	}

	Return::Return(std::unique_ptr<Statement> statement)
		: stmt_{ std::move(statement) }
	{
	}

	ObjectHolder Return::Execute(Closure& closure, Context& context) {
		throw stmt_->Execute(closure, context);
	}

	ClassDefinition::ClassDefinition(ObjectHolder cls)
		: cls_(move(cls))
	{
	}

	ObjectHolder ClassDefinition::Execute(Closure& closure, [[maybe_unused]] Context& context) {
		closure[cls_.TryAs<runtime::Class>()->GetName()] = cls_;
		return ObjectHolder::None();
	}

	FieldAssignment::FieldAssignment(VariableValue object, std::string field_name,
		std::unique_ptr<Statement> rv)
		: object_(move(object))
		, field_name_(move(field_name))
		, rv_(move(rv))
	{
	}

	ObjectHolder FieldAssignment::Execute(Closure& closure, Context& context) {
		runtime::ClassInstance* obj = object_.Execute(closure, context).TryAs<runtime::ClassInstance>();
		if (obj) {
			return obj->Fields()[field_name_] = rv_->Execute(closure, context);
		}
		else {
			throw runtime_error("Object is not class"s);
		}
	}

	IfElse::IfElse(std::unique_ptr<Statement> condition, std::unique_ptr<Statement> if_body,
		std::unique_ptr<Statement> else_body)
		: condition_(move(condition))
		, if_body_(move(if_body))
		, else_body_(move(else_body))
	{
	}

	ObjectHolder IfElse::Execute(Closure& closure, Context& context) {
		ObjectHolder if_cnd = condition_->Execute(closure, context);
		if (if_cnd.TryAs<runtime::Bool>()->GetValue()) {
			if_body_->Execute(closure, context);
		}
		else if (else_body_ != nullptr) {
			ObjectHolder else_cnd = else_body_->Execute(closure, context);
		}

		return ObjectHolder::None();
	}

	ObjectHolder Or::Execute(Closure& closure, Context& context) {
		runtime::ObjectHolder lhs = lhs_->Execute(closure, context);
		runtime::Bool* l = lhs.TryAs<runtime::Bool>();

		if (l) {
			if (!l->GetValue()) {
				runtime::ObjectHolder rhs = rhs_->Execute(closure, context);
				runtime::Bool* r = rhs.TryAs<runtime::Bool>();
				if (r) {
					return ObjectHolder::Own<runtime::Bool>(r->GetValue());
				}
				throw std::runtime_error("Cannot execute logic binary operation"s);
			}
			return ObjectHolder::Own<runtime::Bool>(true);
		}

		throw std::runtime_error("Cannot execute logic binary operation"s);
	}

	ObjectHolder And::Execute(Closure& closure, Context& context) {
		runtime::ObjectHolder lhs = lhs_->Execute(closure, context);
		runtime::Bool* l = lhs.TryAs<runtime::Bool>();

		if (l) {
			if (l->GetValue()) {
				runtime::ObjectHolder rhs = rhs_->Execute(closure, context);
				runtime::Bool* r = rhs.TryAs<runtime::Bool>();
				if (r) {
					return ObjectHolder::Own<runtime::Bool>(r->GetValue());
				}
				throw std::runtime_error("Cannot execute logic binary operation"s);
			}
			return ObjectHolder::Own<runtime::Bool>(false);
		}

		throw std::runtime_error("Cannot execute logic binary operation"s);
	}

	ObjectHolder Not::Execute(Closure& closure, Context& context) {
		runtime::ObjectHolder arg = arg_->Execute(closure, context);
		runtime::Bool* a = arg.TryAs<runtime::Bool>();
		if (a) {
			return ObjectHolder::Own<runtime::Bool>(!a->GetValue());
		}

		throw std::runtime_error("Cannot execute unary operation"s);
	}

	Comparison::Comparison(Comparator cmp, unique_ptr<Statement> lhs, unique_ptr<Statement> rhs)
		: BinaryOperation(std::move(lhs), std::move(rhs))
		, cmp_(cmp)
	{
	}

	ObjectHolder Comparison::Execute(Closure& closure, Context& context) {
		bool res = cmp_(lhs_->Execute(closure, context), rhs_->Execute(closure, context), context);
		return  ObjectHolder::Own<runtime::Bool>(res);
	}

	NewInstance::NewInstance(const runtime::Class& class_, std::vector<std::unique_ptr<Statement>> args)
		: class__({ class_ })
		, args_(move(args))
	{
	}

	NewInstance::NewInstance(const runtime::Class& class_)
		: class__({ class_ })
	{
	}

	ObjectHolder NewInstance::Execute(Closure& closure, Context& context) {
		if (class__.HasMethod(INIT_METHOD, args_.size())) {
			vector<runtime::ObjectHolder> args;
			for (auto& arg : args_) {
				args.push_back(arg->Execute(closure, context));
			}
			class__.Call(INIT_METHOD, args, context);
		}

		return ObjectHolder::Share(class__);
	}

	MethodBody::MethodBody(std::unique_ptr<Statement>&& body)
		: body_(forward<unique_ptr<Statement>>(body)) 
	{
	}

	ObjectHolder MethodBody::Execute(Closure& closure, Context& context) {
		try {
			body_->Execute(closure, context);
			return runtime::ObjectHolder::None();
		}
		catch (runtime::ObjectHolder& result) {
			return result;
		}
		catch (...) {
			throw;
		}
	}

	UnaryOperation::UnaryOperation(std::unique_ptr<Statement> argument)
		: arg_(move(argument))
	{
	}

	BinaryOperation::BinaryOperation(std::unique_ptr<Statement> lhs, std::unique_ptr<Statement> rhs)
		: lhs_(move(lhs))
		, rhs_(move(rhs))
	{
	}


}  // namespace ast