// struct index provides a simple data structure to store extracted typedefs and functions
// from an Abstract Syntax Tree (AST).
//
// The Index struct holds slices/maps for functions and typedefs, which can be populated by
// parsing the AST and extracting relevant information.
package ast

type Index struct {
	Functions []string
	Typedefs  map[string][]IndexMemberDecl
	Structs   map[string][]IndexMemberDecl
}

type IndexMemberDecl struct {
	Name     string
	TypeName string
}
