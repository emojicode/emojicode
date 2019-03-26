from subprocess import *
import glob
import os
import dist
import sys
import re

quick = len(sys.argv) > 1 and sys.argv[1] == 'quick'

compilation_tests = [
    "hello",
    "print",
    "intTest",
    "if",
    "vars",
    "repeatedPackageImport",
    "enum",
    "enumMethod",
    "enumTypeMethod",
    "unwrap",
    "imperativeInterrogative",
    "assignmentMethod",
    "assignmentByCall",
    "repeatWhile",
    "conditionalProduce",
    "stringConcat",
    "babyBottleInitializer",
    "classInheritance",
    "classOverride",
    "classSuper",
    "classSubInstanceVar",
    "optionalParameter",
    "returnInBlock",
    "returnInIf",
    "identityOperator",
    "typesAsValues",
    "class",
    "ivarAssign",
    "useAndAssign",
    "privateClassMethod",
    "assignmentByCallInstanceVariable",
    "valueType",
    "valueTypeSelf",
    "valueTypeMutate",
    "compareNoValue",
    "downcastClass",
    "castAny",
    "castGenericValueType",
    "castGenericClass",
    "protocolClass",
    "protocolSubclass",
    "protocolValueType",
    "protocolValueTypeRemote",
    "protocolEnum",
    "protocolGenericLayerClass",
    "protocolGenericLayerValueType",
    "protocolMulti",
    "reboxToSomething",
    "assignmentByCallProtocol",
    "commonType",
    "generics",
    "genericsValueType",
    "genericProtocol",
    "genericProtocolValueType",
    "genericTypeMethod",
    "genericLocalAsArgToGeneric",
    "genericToConstraintOptional",
    "variableInitAndScoping",
    "varInitPath",
    "valueTypeRemoteAdditional",
    "closureBasic",
    "closureCapture",
    "closureCaptureThis",
    "closureCaptureValueType",
    "closureCaptureThisClass",
    "closureCaptureNonEscaping",
    "closureError",
    "callableBoxing",
    "errorUnwrap",
    "errorAvocado",
    "errorInitializer",
    "errorReraiseMem",
    "errorReraiseMem2",
    "valueTypeCopySelf",
    "valueTypeBoxCopySelf",
    "includer",
    "threads",
    "inferListLiteralFromExpec",
    "sequenceTypeNames",
    "typeValues",
    "rcOrder",
    "rcOrderVt",
    "rcTempOrder",
    "rcInstanceVariable",
    "rcOnlyReference",
    "references",
    "identifierTest",
    "shortCircuit",
    "errorReraisePrefix",
]

if not quick:
    compilation_tests.extend([
      "stressTest1",
      "stressTest2",
      "stressTest3",
      "stressTest4"
    ])

library_tests = [
    "primitives",
    "mathTest",
    "rangeTest",
    "stringTest",
    "dataTest",
    "systemTest",
    "listTest",
    "enumerator",
    "dictionaryTest",
    "jsonTest",
    "fileTest"
]
reject_tests = glob.glob(os.path.join(dist.source, "tests", "reject",
                                      "*.emojic"))

failed_tests = []

emojicodec = os.path.abspath("Compiler/emojicodec")
os.environ["EMOJICODE_PACKAGES_PATH"] = os.path.abspath(".")


def fail_test(name):
    global failed_tests
    print("🛑 {0} failed".format(name))
    failed_tests.append(name)


def test_paths(name, kind):
    return (os.path.join(dist.source, "tests", kind, name + ".emojic"),
            os.path.join(dist.source, "tests", kind, name))


def library_test(name):
    source_path, binary_path = test_paths(name, 's')

    run([emojicodec, source_path, '-O'], check=True)
    completed = run([binary_path], stdout=PIPE)
    if completed.returncode != 0:
        fail_test(name)
        print(completed.stdout.decode('utf-8'))


def compilation_test(name):
    source_path, binary_path = test_paths(name, 'compilation')
    run([emojicodec, source_path, '-O'], check=True)
    completed = run([binary_path], stdout=PIPE)
    exp_path = os.path.join(dist.source, "tests", "compilation", name + ".txt")
    output = completed.stdout.decode('utf-8')
    if output != open(exp_path, "r", encoding='utf-8').read():
        print(output)
        fail_test(name)


def reject_test(filename):
    completed = run([emojicodec, filename], stderr=PIPE)
    output = completed.stderr.decode('utf-8')
    if completed.returncode != 1 or len(re.findall(r"🚨 error:", output)) != 1:
        print(output)
        fail_test(filename)


def available_compilation_tests():
    paths = glob.glob(os.path.join(dist.source, "tests", "compilation",
                                   "*.emojic"))

    map_it = map(lambda f: os.path.splitext(os.path.basename(f))[0], paths)
    tests = list(map_it)
    tests.remove('included')
    return tests


avl_compilation_tests = available_compilation_tests()


def prettyprint_test(name):
    source_path = test_paths(name, 'compilation')[0]
    run([emojicodec, '--format', source_path], check=True)
    try:
        compilation_test(name)
    except CalledProcessError:
        fail_test(name)
    os.rename(source_path + '_original', source_path)


for test in compilation_tests:
    avl_compilation_tests.remove(test)
    compilation_test(test)

if not quick:
    for test in compilation_tests:
        prettyprint_test(test)

    included = os.path.join(dist.source, "tests", "compilation", "included.emojic")
    os.rename(included + '_original', included)

for test in reject_tests:
    reject_test(test)
os.chdir(os.path.join(dist.source, "tests", "s"))
os.environ["TEST_ENV_1"] = "The day starts like the rest I've seen"
for test in library_tests:
    library_test(test)

for file in avl_compilation_tests:
    print("☢️  {0} is not in compilation test list.".format(file))

if len(failed_tests) == 0:
    print("✅ ✅  All tests passed.")
    sys.exit(0)
else:
    print("🛑 🛑  {0} tests failed: {1}".format(len(failed_tests),
                                              ", ".join(failed_tests)))
    sys.exit(1)
