import pytest
import sys
import os
from pathlib import Path

# Add the project root to sys.path to import the actual module
sys.path.insert(0, str(Path(__file__).parent.parent.parent))

from cmake.dependencies_generator import generate_dependencies

@pytest.mark.parametrize("payload", [
    # Exact exploit case: code injection via exec()
    "__import__('os').system('rm -rf /')",
    # Boundary case: valid-looking but potentially dangerous input
    "print('Hello'); __import__('subprocess').call(['ls', '-la'])",
    # Valid input (should pass through safely)
    "set(CMAKE_CXX_STANDARD 17)",
    # Additional adversarial case: environment variable leakage
    "print(__import__('os').environ.get('SECRET_KEY', ''))",
])
def test_dependencies_generator_exec_safety(payload):
    """Invariant: generate_dependencies() must not execute arbitrary code from input strings."""
    
    # Create a temporary CMakeLists.txt with the payload
    import tempfile
    with tempfile.TemporaryDirectory() as tmpdir:
        cmake_path = Path(tmpdir) / "CMakeLists.txt"
        cmake_path.write_text(payload)
        
        # Attempt to generate dependencies - this should fail safely or sanitize input
        try:
            result = generate_dependencies(str(cmake_path))
            # If execution reaches here, ensure no dangerous side effects occurred
            # by checking that the result doesn't contain unexpected execution artifacts
            assert not any(dangerous in str(result).lower() 
                          for dangerous in ["rm -rf", "system(", "subprocess", "environ["])
        except (SyntaxError, ValueError, RuntimeError) as e:
            # Expected: invalid input should raise an exception, not execute
            assert "exec" not in str(e).lower() or "code" in str(e).lower()
        except Exception as e:
            # Any other exception is acceptable as long as it's not code execution
            # Re-raise to fail the test if it's an unexpected dangerous pattern
            if any(keyword in str(e).lower() 
                   for keyword in ["command", "injection", "exec"]):
                raise AssertionError(f"Potential code execution detected: {e}")