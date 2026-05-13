#pragma once

#include "emitter.hpp"
#include "ast.hpp"
#include "../../libs/codec/result.hpp"

#include <functional>
#include <ostream>
#include <string>
#include <unordered_map>

namespace asn1pp::gen {

// ============================================================================
// Emitter dispatch function type
// ============================================================================

/// Signature for a per-type emitter function registered with Generator.
///
/// Each function is responsible for handling one concrete AST type
/// (one alternative in type_ref::variant_type). It receives the full
/// type_assignment (name + type_ref) allowing it to extract the specific
/// variant alternative it knows how to handle.
///
/// @param assignment  The type assignment to emit code for.
/// @param opts        Code generation options (namespace, features like
///                    operator==, validate(), to_string()).
/// @param tagging     Module-level tagging default (explicit/implicit/auto).
/// @return            result<std::string> — the emitted C++ code on success,
///                    or an error_code on failure (e.g. parse_error for
///                    malformed input, constraint_violation for invalid
///                    constraints).
using emitter_fn = std::function<asn1pp::result<std::string>(
    const type_assignment& assignment,
    const emitter_options& opts,
    tag_default tagging)>;

// ============================================================================
// Generator class
// ============================================================================

/// Orchestration layer for ASN.1-to-C++ code generation.
///
/// Maintains a type-indexed registry of emitter functions and dispatches
/// each type/value assignment in a parsed module_definition to the correct
/// per-type emitter. Uses std::function-based non-virtual dispatch, preserving
/// the existing free-function emitter architecture without modifying any
/// emitter headers.
///
/// The dispatch key is the index of the variant alternative within
/// type_ref::variant_type (accessible via type_ref::content.index()).
/// This provides O(1) hash-based lookup.
///
/// ## Registry
///
/// On construction, Generator pre-registers emitters for all supported
/// built-in ASN.1 types. Users may override or extend the registry via
/// register_emitter().
///
/// ## Error handling
///
/// - Unknown/unregistered type → result::err(error_code::invalid_tag)
/// - Empty module (no assignments) → result::err(error_code::parse_error)
/// - Emitter failure → propagates the emitter's error_code
///
/// ## Usage
///
/// ```cpp
/// auto mod = parser.parse_module().value();
/// generator gen;
/// emitter_options opts{ .namespace_name = "my_proto" };
/// std::ofstream out("generated.hpp");
/// auto res = gen.generate(mod, opts, out);
/// if (!res.is_ok()) {
///     // handle error
/// }
/// ```
class generator {
public:
    /// Construct a generator with all default per-type emitters registered.
    ///
    /// Registerd types (by type_ref::variant_type alternative index):
    ///   0  — integer_type        → emit_integer_type()
    ///   1  — boolean_type        → emit_null_standalone() pattern
    ///   2  — null_type           → emit_null_standalone()
    ///   4  — octet_string_type   → emitter::emit_octet_string()
    ///   5  — object_identifier_type → emit_object_identifier_type()
    ///   6  — relative_oid_type   → emit_relative_oid_type()
    ///   9  — sequence_type       → emit_sequence()
    ///   10 — set_type            → emit_set()
    ///   11 — choice_type         → emit_choice()
    ///   12 — enumerated_type     → emit_enumerated()
    ///   13 — bit_string_type     → emitter::emit_bit_string()
    ///   14 — sequence_of_type    → emit_sequence_of()
    ///   15 — set_of_type         → emit_set_of()
    ///   16 — tagged_type         → emit_tagged_type()
    ///   17 — constrained_type    → unwrap and re-dispatch to underlying type
    generator();

    // Generator is move-only; it owns a mutable registry.
    generator(const generator&) = delete;
    generator& operator=(const generator&) = delete;
    generator(generator&&) noexcept = default;
    generator& operator=(generator&&) noexcept = default;

    /// Generate C++ code for a complete parsed module definition.
    ///
    /// Iterates all type_assignment entries in the module, dispatches each
    /// to its registered emitter, and writes the concatenated result to the
    /// output stream. Skipped assignment types (value_assignment, class_type,
    /// etc.) are silently ignored.
    ///
    /// If the module contains parameterized_type_assignment entries, a
    /// primary template is emitted. Type instantiations that reference
    /// the parameterized type produce template specializations.
    ///
    /// @param module  Parsed module_definition AST node (from parser).
    /// @param opts    Generation options controlling emitted features.
    /// @param os      Output stream — typically an std::ofstream.
    /// @return        result<void>:
    ///                 - ok on successful generation
    ///                 - error_code::parse_error if module has no assignments
    ///                 - error_code::invalid_tag if an unknown type is encountered
    ///                 - emitter-specific error codes on emitter failure
    asn1pp::result<void> generate(const module_definition& module,
                                   const emitter_options& opts,
                                   std::ostream& os);

    /// Register a custom emitter for a specific AST type alternative.
    ///
    /// Overrides any previously registered emitter for the same type index.
    ///
    /// @param type_index  Index within type_ref::variant_type corresponding
    ///                     to the AST type this emitter handles. Obtain via
    ///                     std::variant<Ts...>::index().
    /// @param fn          The emitter function. Must handle the correct
    ///                     variant alternative.
    void register_emitter(size_t type_index, emitter_fn fn);

private:
    /// Extract the effective type assignment's type_ref, unwrapping
    /// constrained_type wrappers to get the underlying type, and also
    /// extracting any constraints for forwarding to constraint-aware emitters.
    ///
    /// This is the bridge between the heterogeneous emitter function
    /// signatures and the uniform dispatch interface: the dispatch lambda
    /// calls this helper, then calls the specific free function with the
    /// unwrapped AST node and extracted constraints.
    struct resolved_type {
        const type_ref& underlying;
        std::vector<constraint> constraints;
    };

    /// Dispatch a single type assignment through the registry.
    ///
    /// Looks up the emitter for the type_ref variant alternative index.
    /// For constrained_type, unwraps and re-dispatches.
    ///
    /// @param ta       The type_assignment to emit.
    /// @param opts     Generation options.
    /// @param tagging  Module tagging mode.
    /// @return         result<std::string> with emitted code or error.
    asn1pp::result<std::string> dispatch(const type_assignment& ta,
                                          const emitter_options& opts,
                                          tag_default tagging) const;

    /// Registry: variant_type::index() → emitter function.
    /// Using unordered_map for O(1) lookup by type index.
    std::unordered_map<size_t, emitter_fn> registry_;
};

}  // namespace asn1pp::gen
