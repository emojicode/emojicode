from subprocess import *
import glob
import os
import dist
import sys
import re

compilation_tests = [
    "hello",
    "intTest",
    "if",
    "namespace",
    "enum",
    "enumMethod",
    "enumTypeMethod",
    "unwrap",
    "assignmentByCall",
    "repeatWhile",
    "conditionalProduce",
    "stringConcat",
    "classInheritance",
    "classOverride",
    "classSuper",
    "classSubInstanceVar",
    "extension",
    "piglatin",
    "class",
    "assignmentByCallInstanceVariable",
    "babyBottleInitializer",
    "valueType",
    "valueTypeSelf",
    "valueTypeMutate",
    "isNothingness",
    "downcastClass",
    "castAny",
    "castGenericValueType",
    "protocolClass",
    "protocolSubclass",
    "protocolValueType",
    "protocolValueTypeRemote",
    "protocolEnum",
    "protocolGenericLayerClass",
    "protocolGenericLayerValueType",
    "protocolMulti",
    "assignmentByCallProtocol",
    "commonType",
    "typeAlias",
    "generics",
    "genericsValueType",
    "genericProtocol",
    "genericProtocolValueType",
    "variableInitAndScoping",
    "valueTypeRemoteAdditional",
    "closureBasic",
    "closureCapture",
    "closureCaptureValueType",
    "captureMethod",
    "captureTypeMethod",
    "errorIsError",
    "errorPerfect",
    "errorAvocado",
    "errorInitializer",
    "gcStressTest1",
    "gcStressTest2",
    "gcStressTest3",
    "valueTypeCopySelf",
    "valueTypeBoxCopySelf",
    "includer",
    "threads",
]
library_tests = [
    "stringTest", "primitives", "mathTest", "listTest", "rangeTest",
    "dataTest", "dictionaryTest", "systemTest", "jsonTest", "enumerator",
    "fileTest"
]
reject_tests = glob.glob(os.path.join(dist.source, "tests", "reject",
                                      "*.emojic"))

failed_tests = []

emojicode = os.path.abspath("emojicode")
emojicodec = os.path.abspath("emojicodec")
os.environ["EMOJICODE_PACKAGES_PATH"] = os.path.join(dist.path, "packages")


def fail_test(name):
    global failed_tests
    print("🛑 {0} failed".format(name))
    failed_tests.append(name)


def test_paths(name, kind):
    return (os.path.join(dist.source, "tests", kind, name + ".emojic"),
            os.path.join(dist.source, "tests", kind, name + ".emojib"))


def library_test(name):
    source_path, binary_path = test_paths(name, 's')

    run([emojicodec, source_path], check=True)
    completed = run([emojicode, binary_path], stdout=PIPE)
    if completed.returncode != 0:
        fail_test(name)
        print(completed.stdout.decode('utf-8'))


def compilation_test(name):
    source_path, binary_path = test_paths(name, 'compilation')

    run([emojicodec, source_path], check=True)
    completed = run([emojicode, binary_path], stdout=PIPE)
    exp_path = os.path.join(dist.source, "tests", "compilation", name + ".txt")
    output = completed.stdout.decode('utf-8')
    if output != open(exp_path, "r", encoding='utf-8').read():
        print(output)
        fail_test(name)


def reject_test(filename):
    completed = run([emojicodec, filename], stderr=PIPE)
    output = completed.stderr.decode('utf-8')
    if completed.returncode == 0 or len(re.findall(r"(^|\n)🚨", output)) != 1:
        print(output)
        fail_test(filename)


for test in compilation_tests:
    compilation_test(test)
for test in reject_tests:
    reject_test(test)
os.chdir(os.path.join(dist.source, "tests", "s"))
for test in library_tests:
    library_test(test)

if len(failed_tests) == 0:
    print("✅ ✅  All tests passed.")
    sys.exit(0)
else:
    print("🛑 🛑  {0} tests failed: {1}".format(len(failed_tests),
                                              ", ".join(failed_tests)))
    sys.exit(1)
