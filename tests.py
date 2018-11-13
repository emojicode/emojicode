from subprocess import *
import glob
import os
import dist
import sys
import re

quick = len(sys.argv) > 2 and sys.argv[2] == 'quick'

compilation_tests = [
    "hello.emojic",
    "hello.🍇",
    "print.emojic",
    "intTest.emojic",
    "if.emojic",
    "vars.emojic",
    "repeatedPackageImport.emojic",
    "enum.emojic",
    "enumMethod.emojic",
    "enumTypeMethod.emojic",
    "unwrap.emojic",
    "imperativeInterrogative.emojic",
    "assignmentByCall.emojic",
    "repeatWhile.emojic",
    "conditionalProduce.emojic",
    "stringConcat.emojic",
    "babyBottleInitializer.emojic",
    "classInheritance.emojic",
    "classOverride.emojic",
    "classSuper.emojic",
    "classSubInstanceVar.emojic",
    "optionalParameter.emojic",
    "returnInBlock.emojic",
    "returnInIf.emojic",
    "identityOperator.emojic",
    "typesAsValues.emojic",
    "class.emojic",
    "privateClassMethod.emojic",
    # "extension",
    "assignmentByCallInstanceVariable.emojic",
    "valueType.emojic",
    "valueTypeSelf.emojic",
    "valueTypeMutate.emojic",
    "compareNoValue.emojic",
    "downcastClass.emojic",
    "castAny.emojic",
    # "castGenericValueType",
    "protocolClass.emojic",
    "protocolSubclass.emojic",
    "protocolValueType.emojic",
    "protocolValueTypeRemote.emojic",
    "protocolEnum.emojic",
    "protocolGenericLayerClass.emojic",
    "protocolGenericLayerValueType.emojic",
    "protocolMulti.emojic",
    "reboxToSomething.emojic",
    "assignmentByCallProtocol.emojic",
    "commonType.emojic",
    "generics.emojic",
    "genericsValueType.emojic",
    "genericProtocol.emojic",
    "genericProtocolValueType.emojic",
    "genericTypeMethod.emojic",
    "genericLocalAsArgToGeneric.emojic",
    "variableInitAndScoping.emojic",
    "valueTypeRemoteAdditional.emojic",
    "closureBasic.emojic",
    "closureCapture.emojic",
    "closureCaptureThis.emojic",
    "closureCaptureValueType.emojic",
    "closureCaptureThisClass.emojic",
    "callableBoxing.emojic",
    "errorIsError.emojic",
    "errorUnwrap.emojic",
    "errorAvocado.emojic",
    "errorHandlerMemory.emojic",
    "errorInitializer.emojic",
    "valueTypeCopySelf.emojic",
    "valueTypeBoxCopySelf.emojic",
    "includer.emojic",
    "threads.emojic",
    "inferListLiteralFromExpec.emojic",
    "sequenceTypeNames.emojic",
    "typeValues.emojic",
    "rcOrder.emojic",
    "rcOrderVt.emojic",
    "rcTempOrder.emojic",
    "rcInstanceVariable.emojic",
    "rcOnlyReference.emojic",
    "identifierTest.emojic",
]

if not quick:
    compilation_tests.extend([
      "stressTest1.emojic",
      "stressTest2.emojic",
      "stressTest3.emojic",
      "stressTest4.emojic"
    ])

library_tests = [
    "primitives.emojic",
    "mathTest.emojic",
    "rangeTest.emojic",
    "stringTest.emojic",
    "dataTest.emojic",
    "systemTest.emojic",
    "listTest.emojic",
    "enumerator.emojic",
    "dictionaryTest.emojic",
    # "jsonTest",
    "fileTest.emojic"
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
    root, ext = os.path.splitext(name)
    return (os.path.join(dist.source, "tests", kind, name),
            os.path.join(dist.source, "tests", kind, root))


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
    exp_path = os.path.join(dist.source, "tests", "compilation",
                            os.path.splitext(name)[0] + ".txt")
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
    paths.extend(glob.glob(os.path.join(dist.source, "tests", "compilation",
                                   "*.🍇")))

    map_it = map(lambda f: os.path.basename(f), paths)
    tests = list(map_it)
    tests.remove('included.emojic')
    return tests


avl_compilation_tests = available_compilation_tests()


def prettyprint_test(name):
    source_path = test_paths(name, 'compilation')[0]
    run([emojicodec, '--format', source_path], check=True)
    try:
        compilation_test(name)
    except CalledProcessError:
        pass
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
