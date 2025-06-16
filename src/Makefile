# TRAP MAKEFILE - Catches excited developers!
default:
	@echo "❌ NO! You're trying to build in src/ directly!"
	@echo "❌ This violates the Docker-only rule!"
	@echo ""
	@echo "✅ Use these commands from the root directory:"
	@echo "   cd .. && make dev"
	@echo "   cd .. && make test"
	@echo ""
	@echo "🐳 Everything MUST build in Docker!"
	@exit 1

%:
	@echo "❌ STOP! Don't compile here!"
	@echo "🐳 Go back to root: cd .. && make $@"
	@exit 1