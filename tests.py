from subprocess import *
import glob
import os
import dist
import sys
import re

quick = len(sys.argv) > 1 and sys.argv[1] == 'quick'
valgrind = len(sys.argv) > 1 and sys.argv[1] == 'valgrind'

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
    "overload",
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
    "genericsInferenceValueType",
    "genericsInferenceClass",
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
    "inferLiteralFromExpec",
    "sequenceTypeNames",
    "typeValues",
    "deinitializer",
    "rcOrder",
    "rcOrderVt",
    "rcTempOrder",
    "rcInstanceVariable",
    "rcOnlyReference",
    "rcIvarArgMut",
    "rcEscaping",
    "classEscapingParamOverride",
    "references",
    "identifierTest",
    "shortCircuit",
    "errorReraisePrefix",
    "weak",
    "superMemoryFlow",
    "interpolationDereference"
]

if not (quick or valgrind):
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
package_tests = [
    ("packageImportingPackage", ["inner", "outer"])
]

failed_tests = []

emojicodec = os.path.abspath("Compiler/emojicodec")
os.environ["EMOJICODE_PACKAGES_PATH"] = os.path.abspath(".")
test_packages_path = os.path.abspath(os.path.join("tests", "packages"))


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


def build_test_package(name):
    source_path = os.path.join(dist.source, "tests", "packages", name,
                               name + ".emojic")
    out_dir = os.path.join(test_packages_path, name)
    os.makedirs(out_dir, exist_ok=True)
    run([emojicodec, source_path, '-p', name, '-S', test_packages_path,
         '-o', os.path.join(out_dir, "lib" + name + ".a")], check=True)


def package_test(name, packages):
    for package in packages:
        build_test_package(package)
    source_path = os.path.join(dist.source, "tests", "packages",
                               name + ".emojic")
    binary_path = os.path.join(test_packages_path, name)
    run([emojicodec, source_path, '-S', test_packages_path,
         '-o', binary_path], check=True)
    completed = run([binary_path], stdout=PIPE)
    exp_path = os.path.join(dist.source, "tests", "packages", name + ".txt")
    output = completed.stdout.decode('utf-8')
    if output != open(exp_path, "r", encoding='utf-8').read():
        print(output)
        fail_test(name)


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


def test():
    for test in compilation_tests:
        avl_compilation_tests.remove(test)
        compilation_test(test)

    if not quick:
        for test in compilation_tests:
            prettyprint_test(test)

        included = os.path.join(dist.source, "tests", "compilation", "included.emojic")
        os.rename(included + '_original', included)

        source_path = test_paths('included', 'compilation')[0]
        run([emojicodec, '--format', source_path], check=True)
        compilation_test('includer')
        os.rename(source_path + '_original', source_path)

    for test in reject_tests:
        reject_test(test)
    for name, packages in package_tests:
        package_test(name, packages)
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


def run_valgrind():
    for test in compilation_tests:
        source_path, binary_path = test_paths(test, 'compilation')
        run([emojicodec, source_path, '-O'], check=True)
        completed = run(['valgrind', '--error-exitcode=22', '--leak-check=full', binary_path], stdout=PIPE, stderr=PIPE)
        if completed.returncode == 22:
            print(completed.stdout.decode('utf-8'))
            print(completed.stderr.decode('utf-8'))
            fail_test(test)

if valgrind:
    run_valgrind()
else:
    test()

