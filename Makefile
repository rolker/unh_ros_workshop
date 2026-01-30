.PHONY: all init humble underlay bootcamp clean clean-humble clean-underlay clean-bootcamp help

all: bootcamp

init:
	@if [ -f humble_ws/src/.vcs_import_done ]; then \
		echo "Humble workspace already initialized, skipping"; \
	else \
		bash scripts/initialize_humble_workspace.bash && touch humble_ws/src/.vcs_import_done; \
	fi

humble: init
	@if [ -f humble_ws/install/setup.bash ]; then \
		echo "Humble workspace already built, skipping"; \
	else \
		bash scripts/build_humble_ws.bash; \
	fi

underlay: humble
	@bash scripts/build_underlay_ws.bash

bootcamp: underlay
	@echo "=========================================="
	@echo "Building bootcamp workspace"
	@echo "=========================================="
	@bash -c "source humble_ws/install/setup.bash && source underlay_ws/install/setup.bash && cd ros_bootcamp_ws && echo 'Building workspace in: $$(pwd)' && colcon build --symlink-install && echo '==========================================' && echo 'Bootcamp workspace build complete!' && echo 'Source with: source ros_bootcamp_ws/install/setup.bash' && echo '=========================================='"

clean-humble:
	@echo "Cleaning humble workspace..."
	@rm -rf humble_ws/build humble_ws/install humble_ws/log

clean-underlay:
	@echo "Cleaning underlay workspace..."
	@rm -rf underlay_ws/build underlay_ws/install underlay_ws/log

clean-bootcamp:
	@echo "Cleaning bootcamp workspace..."
	@rm -rf ros_bootcamp_ws/build ros_bootcamp_ws/install ros_bootcamp_ws/log

clean: clean-bootcamp clean-underlay
	@echo "Cleaned underlay and bootcamp workspaces"

help:
	@echo "Available targets:"
	@echo "  all            - Build everything (init -> humble -> underlay -> bootcamp)"
	@echo "  init           - Initialize humble workspace (download sources)"
	@echo "  humble         - Build ROS2 Humble from source (runs init first, skips if built)"
	@echo "  underlay       - Build underlay workspace (runs humble first)"
	@echo "  bootcamp       - Build bootcamp workspace (runs underlay first)"
	@echo "  clean          - Clean underlay and bootcamp build artifacts"
	@echo "  clean-humble   - Clean humble workspace build artifacts"
	@echo "  clean-underlay - Clean underlay workspace build artifacts"
	@echo "  clean-bootcamp - Clean bootcamp workspace build artifacts"
	@echo ""
	@echo "Typical workflow:"
	@echo "  make all       # Builds everything in correct order"
	@echo "  make bootcamp  # Same as 'make all'"
	@echo ""
	@echo "Note: 'make humble' skips if already built; underlay/bootcamp always rebuild (fast)"
