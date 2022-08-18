#include "runtime.h"


using namespace std;

namespace runtime {

	ObjectHolder::ObjectHolder(std::shared_ptr<Object> data)
		: data_(std::move(data)) {
	}

	void ObjectHolder::AssertIsValid() const {
		assert(data_ != nullptr);
	}

	ObjectHolder ObjectHolder::Share(Object& object) {
		// Возвращаем невладеющий shared_ptr (его deleter ничего не делает)
		return ObjectHolder(std::shared_ptr<Object>(&object, [](auto* /*p*/) { /* do nothing */ }));
	}

	ObjectHolder ObjectHolder::None() {
		return ObjectHolder();
	}

	Object& ObjectHolder::operator*() const {
		AssertIsValid();
		return *Get();
	}

	Object* ObjectHolder::operator->() const {
		AssertIsValid();
		return Get();
	}

	Object* ObjectHolder::Get() const {
		return data_.get();
	}

	ObjectHolder::operator bool() const {
		return Get() != nullptr;
	}

	bool IsTrue(const ObjectHolder& object) {

		Bool* is_bool = object.TryAs<Bool>();
		if (is_bool)
		{
			return is_bool->GetValue();
		}

		Number* is_number = object.TryAs<Number>();
		if (is_number) {
			return !(is_number->GetValue() == 0);
		}

		String* is_string = object.TryAs<String>();
		if (is_string) {
			return !(is_string->GetValue().empty());
		}

		return false;
	}







	void ClassInstance::Print(std::ostream& os, Context& context) {
		if (HasMethod(STR_METHOD, 0U)) {
			Call(STR_METHOD, {}, context)->Print(os, context);
		}
		else {
			os << this;
		}
	}

	bool ClassInstance::HasMethod(const std::string& method, size_t argument_count) const {
		const Method* mt = class_.GetMethod(method);
		if (mt != nullptr) {
			return mt->formal_params.size() == argument_count;
		}
		return false;
	}

	Closure& ClassInstance::Fields() {
		return closure_;
	}

	const Closure& ClassInstance::Fields() const {
		return closure_;
	}

	ClassInstance::ClassInstance(const Class& cls)
		: class_(cls)
	{
	}

	ObjectHolder ClassInstance::Call(const std::string& method,
		const std::vector<ObjectHolder>& actual_args,
		Context& context) {

		const Method* mt = class_.GetMethod(method);

		if (mt != nullptr && mt->formal_params.size() == actual_args.size()) {
			Closure args;
			args["self"s] = ObjectHolder::Share(*this);

			size_t arg_index = 0;
			for (auto& param : mt->formal_params) {
				args[param] = actual_args.at(arg_index++);
			}

			return mt->body->Execute(args, context);
		}

		throw std::runtime_error("Not implemented"s);
	}







	Class::Class(std::string name, std::vector<Method> methods, const Class* parent)
		: parent_(parent)
		, name_(move(name))
		, methods_(move(methods))
	{
		// Все методы проиндексируем указателями, для ускорения поиска нужных методов
		for (Method& mt : methods_) {
			vt_methods_[mt.name] = &mt;
		}

	}

	const Method* Class::GetMethod(const std::string& name) const {
		// Сначала ищем методы в текущем классе
		auto it = vt_methods_.find(name);
		if (it != vt_methods_.end()) {
			return it->second;
		}

		// Когда нужного метода нет - ищем в родительском
		if (parent_) {
			auto itP = parent_->vt_methods_.find(name);
			if (itP != parent_->vt_methods_.end()) {
				return itP->second;
			}
		}
		return nullptr;
	}

	[[nodiscard]] const std::string& Class::GetName() const {
		return name_;
	}

	void Class::Print(ostream& os, [[maybe_unused]] Context& context) {
		os << "Class " << name_;
	}






	void Bool::Print(std::ostream& os, [[maybe_unused]] Context& context) {
		os << (GetValue() ? "True"sv : "False"sv);
	}





	bool Equal(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
		// Если lhs - объект с методом __eq__, функция возвращает результат вызова lhs.__eq__(rhs)
		auto lhs_is_class = lhs.TryAs<ClassInstance>();
		if (lhs_is_class) {
			return IsTrue(lhs_is_class->Call(EQ_METHOD, { rhs }, context));
		}

		optional<bool> result = Comparator(lhs, rhs, std::equal_to());
		if (result) {
			return result.value();
		}

		if (lhs.Get() == nullptr && rhs.Get() == nullptr) {
			return true;
		}

		throw std::runtime_error("Cannot compare objects for equality"s);
	}

	bool Less(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
		// Если первый аргумент — объект пользовательского класса с методом __lt__, 
		// функция возвращает результат вызова lhs.__lt__(rhs), приведённый к типу Bool

		auto lhs_is_class = lhs.TryAs<ClassInstance>();
		if (lhs_is_class) {
			return IsTrue(lhs_is_class->Call(LESS_METHOD, { rhs }, context));
		}

		optional<bool> result = Comparator(lhs, rhs, std::less());
		if (result) {
			return result.value();
		}

		throw std::runtime_error("Cannot compare objects for less"s);
	}

	bool NotEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
		return !Equal(lhs, rhs, context);
	}

	bool Greater(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
		return !(Less(lhs, rhs, context) || Equal(lhs, rhs, context));
	}

	bool LessOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
		return !Greater(lhs, rhs, context);
	}

	bool GreaterOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
		return !Less(lhs, rhs, context);
	}

}  // namespace runtime