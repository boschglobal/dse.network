package ast

type YAMLVisitor interface {
	Visit(map[string]interface{}, *Index)
}

type InnerVisitor struct {
	YAMLVisitor
}

func (v InnerVisitor) Visit(node map[string]interface{}, idx *Index) {
	innerNodes, ok := node["inner"].([]interface{})
	if !ok {
		return
	}

	for _, innerNode := range innerNodes {
		if innerNodeMap, ok := innerNode.(map[string]interface{}); ok {
			v.YAMLVisitor.Visit(innerNodeMap, idx)
		}
	}
}

type FunctionDeclVisitor struct{}

func (v FunctionDeclVisitor) Visit(node map[string]interface{}, idx *Index) {
	kind, kindOk := node["kind"].(string)
	if !kindOk || kind != "FunctionDecl" {
		return
	}

	name, nameOk := node["name"].(string)
	if nameOk {
		idx.Functions = append(idx.Functions, name)
	}
}

type TypedefDeclVisitor struct {
	TypeList []string
}

func (v *TypedefDeclVisitor) Visit(node map[string]interface{}, idx *Index) {
	kind, kindOk := node["kind"].(string)
	if !kindOk || kind != "TypedefDecl" {
		return
	}

	name, nameOk := node["name"].(string)
	if nameOk {
		v.TypeList = append(v.TypeList, name)
	}
}

type RecordDeclVisitor struct {
	TypeList   []string
	StructList []string
}

func (v RecordDeclVisitor) Visit(node map[string]interface{}, idx *Index) {
	kind, kindOk := node["kind"].(string)
	if !kindOk || kind != "RecordDecl" {
		return
	}

	name, nameOk := node["name"].(string)
	inner, innerOk := node["inner"].([]interface{})
	if !innerOk {
		return
	}
	if nameOk {
		if itemExists(v.TypeList, name) {
			// Record -> Typedef
			for _, innerItem := range inner {
				if innerItemMap, innerItemOk := innerItem.(map[string]interface{}); innerItemOk {
					v.processTypedefFieldDecl(name, innerItemMap, idx)
				}
			}
		} else {
			// Record -> Struct
			v.StructList = append(v.StructList, name)
			for _, innerItem := range inner {
				if innerItemMap, innerItemOk := innerItem.(map[string]interface{}); innerItemOk {
					v.processStructFieldDecl(name, innerItemMap, idx)
				}
			}
		}
	} else {
		return
	}
}

func (v RecordDeclVisitor) processTypedefFieldDecl(typedefName string, innerItemMap map[string]interface{}, idx *Index) {
	innerKind, innerKindOk := innerItemMap["kind"].(string)
	if !innerKindOk || innerKind != "FieldDecl" {
		return
	}

	fieldName, fieldNameOk := innerItemMap["name"].(string)
	fieldTypeMap, fieldTypeMapOk := innerItemMap["type"].(map[string]interface{})
	if !fieldNameOk || !fieldTypeMapOk {
		return
	}

	fieldType, fieldTypeOk := fieldTypeMap["qualType"].(string)
	if fieldTypeOk {
		member := IndexMemberDecl{Name: fieldName, TypeName: fieldType}
		idx.Typedefs[typedefName] = append(idx.Typedefs[typedefName], member)
	}
}

func (v RecordDeclVisitor) processStructFieldDecl(structName string, innerItemMap map[string]interface{}, idx *Index) {
	innerKind, innerKindOk := innerItemMap["kind"].(string)
	if !innerKindOk || innerKind != "FieldDecl" {
		return
	}

	fieldName, fieldNameOk := innerItemMap["name"].(string)
	fieldTypeMap, fieldTypeMapOk := innerItemMap["type"].(map[string]interface{})
	if !fieldNameOk || !fieldTypeMapOk {
		return
	}

	fieldType, fieldTypeOk := fieldTypeMap["qualType"].(string)
	if fieldTypeOk {
		member := IndexMemberDecl{Name: fieldName, TypeName: fieldType}
		idx.Structs[structName] = append(idx.Structs[structName], member)
	}
}

func itemExists(list []string, targetItem string) bool {
	for _, item := range list {
		if item == targetItem {
			return true
		}
	}
	return false
}
