DIRS = chief_actuator_control coarse_metrology coarse_star_tracker \
	  deputy_aux fiber_injection fine_metrology fine_star_tracker \
	  robot_controller science_camera target_server

BUILDDIRS = $(DIRS:%=%/src) plate_solver
CLEANDIRS = $(DIRS:%=clean-%/src) clean-plate_solver
INSTALLDIRS = $(DIRS:%=install-%/src)

.PHONY: all build $(BUILDDIRS) clean $(CLEANDIRS) install $(INSTALLDIRS)

all: build

build: $(BUILDDIRS)

$(BUILDDIRS):
	$(MAKE) -C $@

clean: $(CLEANDIRS)

$(CLEANDIRS): 
	$(MAKE) -C $(@:clean-%=%) clean

install: $(INSTALLDIRS)

$(INSTALLDIRS):
	$(MAKE) -C $(@:install-%=%) install
