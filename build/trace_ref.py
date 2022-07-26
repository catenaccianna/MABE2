import pygame

class AvidaTracer:
    def __init__(self, w, h, is_fullscreen, font_name, font_size, font_color = (255,255,255)):
        self.screen_width = w
        self.screen_height = h
        self.is_fullscreen = is_fullscreen
        if self.is_fullscreen:
            self.screen = pygame.display.set_mode((self.screen_width, self.screen_height),
                    pygame.FULLSCREEN)
        else:
            self.screen = pygame.display.set_mode((self.screen_width, self.screen_height))
        self.clock = pygame.time.Clock()
        self.font = pygame.font.SysFont(font_name, font_size)
        self.font_color = font_color
        self.org_idx = 0
        self.org_step = 0

    def run(self):
        self.is_running = True
        self.is_paused = True
        while self.is_running:
            self.update()
            self.render()
            self.clock.tick(60)

    def update(self):
        for evt in pygame.event.get():
            if evt.type == pygame.QUIT:
                self.is_running = False
            elif evt.type == pygame.KEYDOWN:
                if evt.key == pygame.K_ESCAPE or evt.key == pygame.K_q:
                    self.is_running = False
                elif evt.key == pygame.K_RIGHT or evt.key == pygame.K_d:
                    self.org_step += 1
                    if self.org_step >= len(self.org_states[self.org_idx]):
                        self.org_step = 0
                elif evt.key == pygame.K_LEFT or evt.key == pygame.K_a:
                    self.org_step -= 1
                    if self.org_step < 0:
                        self.org_step = len(self.org_states[self.org_idx]) - 1
                elif evt.key == pygame.K_UP or evt.key == pygame.K_w:
                    self.org_idx -= 1
                    if self.org_idx < 0:
                        self.org_idx = len(self.org_states) - 1 
                    self.org_step = 0
                elif evt.key == pygame.K_DOWN or evt.key == pygame.K_s:
                    self.org_idx += 1
                    if self.org_idx >= len(self.org_states):
                        self.org_idx = 0
                    self.org_step = 0
                elif evt.key == pygame.K_SPACE:
                    self.is_paused = not self.is_paused
        if not self.is_paused:
            self.org_step += 1
            if self.org_step >= len(self.org_states[self.org_idx]):
                self.org_step = 0


    def render(self):
        self.screen.fill((0,0,0))

        self.genome.render(self.screen, self.org_states[self.org_idx][self.org_step])
        
        x = 50
        y = 300
        org_idx_surf = self.font.render('Org idx: ' + str(self.org_idx), 
                True, self.font_color)
        self.screen.blit(org_idx_surf, (x, y))
        y += 30
        org_step_surf = self.font.render('Org step: ' + str(self.org_step), 
                True, self.font_color)
        self.screen.blit(org_step_surf, (x, y))
        y += 60

        x,y = self.render_heads(x, y)
        y += 30
        x,y = self.render_registers(x, y)

        pygame.display.flip()

    def render_heads(self, x, y):
        state = self.org_states[self.org_idx][self.org_step]
        for head in ('IP', 'RH', 'WH', 'FH'):
            surf = self.font.render(head + ': ' + str(state.heads[head]), 
                    True, self.font_color)
            self.screen.blit(surf, (x, y))
            y += 30
        return (x, y)
    
    def render_registers(self, x, y):
        state = self.org_states[self.org_idx][self.org_step]
        for idx in range(len(state.regs)):
            surf = self.font.render('Reg ' + str(idx) + ': ' + str(state.regs[idx]), 
                    True, self.font_color)
            self.screen.blit(surf, (x, y))
            y += 30
        return (x, y)

    
    def set_genome(self, file_name, font, x, y, dx, dy, w, h, angle):
        inst_list = []
        with open(filename, 'r') as fp:
            for line in fp:
                line = line.strip()
                if line != '': 
                    inst_list.append(line)
        self.genome = Genome(inst_list, font, x, y, dx, dy, w, h, angle)

    def parse_mabe_trace(self, filename):
        self.org_states = []
        org_idx = None
        heads = {}
        regs = []
        with open(filename, 'r') as fp:
            for line in fp:
                line = line.strip()
                if len(line) == 0: # Ignore empty lines
                    continue
                line_parts = line.split()
                # Lines that start with [X], where X is a number
                if line_parts[0][0] == '[' and line_parts[0][-1] == ']' \
                        and line_parts[0][1:-1].isnumeric():
                    # Org index line
                    if len(line_parts) == 1:
                        # If we have existing data, save it!
                        if org_idx is not None:
                            while len(self.org_states) <= org_idx:
                                self.org_states.append([])
                            self.org_states[org_idx].append(OrgState(org_idx, heads, regs))
                        # Reset org
                        org_idx = int(line_parts[0][1:-1])
                        heads = {}
                        regs = []
                    # Register line
                    else: 
                        reg_idx = int(line_parts[0][1:-1])
                        while len(regs) <= reg_idx:
                            regs.append(0)
                        regs[reg_idx] = int(line_parts[1]) 
                # Heads line
                elif line_parts[0] == 'IP:':
                    idx = 0
                    while True:
                        if line_parts[idx][-1] == ':' and line_parts[idx][0] != '(' and \
                                idx < len(line_parts) - 1:
                            head_name = line_parts[idx][:-1]
                            heads[head_name] = int(line_parts[idx + 1])
                            idx += 2
                        else:
                            break

            # Save the final org! 
            if org_idx is not None:
                while len(self.org_states) <= org_idx:
                    self.org_states.append([])
                self.org_states[org_idx].append(OrgState(org_idx, heads, regs))
    
    def parse_avida_trace(self, filename):
        self.org_states = []
        org_idx = None
        heads = {}
        regs = []
        with open(filename, 'r') as fp:
            for line in fp:
                line = line.strip()
                if len(line) == 0: # Ignore empty lines
                    continue
                line_parts = line.split()

                # New org line
                if len(line_parts) == 1 and line_parts[0] == 'U:-1':
                    # If we have existing data, save it!
                    if org_idx is not None:
                        while len(self.org_states) <= org_idx:
                            self.org_states.append([])
                        self.org_states[org_idx].append(OrgState(org_idx, heads, regs))
                    # Reset org
                    org_idx = 0
                    heads = {}
                    regs = []
                # CPU line
                if line_parts[0] == 'CPU':
                    for line_part in line_parts:
                        # Instruction pointer
                        if len(line_part) > 3 and line_part[:3] == 'IP:':
                            heads['IP'] = int(line_part[3:])
                        # Registers
                        elif len(line_part) > 3 and line_part[1:3] == 'X:':
                            reg_char = line_part[0]
                            reg_idx = ord(reg_char) - ord('A')
                            reg_val = int(line_part[3:])
                            while len(regs) <= reg_idx:
                                regs.append(0)
                            regs[reg_idx] = reg_val 
                # Heads line
                elif line_parts[0][:7] == 'R-Head:':
                    idx = 0
                    for line_part in line_parts:
                        if len(line_part) >= 8:
                            if line_part[:6] == 'R-Head':
                                heads['RH'] = int(line_part[7:])
                            elif line_part[:6] == 'W-Head':
                                heads['WH'] = int(line_part[7:])
                            elif line_part[:6] == 'F-Head':
                                heads['FH'] = int(line_part[7:])

            # Save the final org! 
            if org_idx is not None:
                while len(self.org_states) <= org_idx:
                    self.org_states.append([])
                self.org_states[org_idx].append(OrgState(org_idx, heads, regs))
    
    def compare_org_trace(self, other, org_idx):
        for step in range(len(self.org_states[org_idx])):
            if step >= len(other.org_states[org_idx]):
                print('Reached end of steps in other trace at step:', step)
                return step
            self_state = self.org_states[org_idx][step]
            other_state = other.org_states[org_idx][step]
            for reg_idx in range(6):#range(len(self.org_states[org_idx][step])):
                if self_state.regs[reg_idx] != other_state.regs[reg_idx]:
                    print('Mismatch in register', reg_idx, 'at step:', step)
                    return step
            for head_name in self_state.heads.keys():
                if head_name not in other_state.heads.keys():
                    print('Head', head_name, 'missing in second trace at step:', step)
                    return step
                if self_state.heads[head_name] != other_state.heads[head_name]:
                    print('Mismatch in head', head_name, 'at step:', step)
                    return step



class Genome:
    def __init__(self, inst_list, font, x, y, dx, dy, w, h, angle):
        self.inst_list = inst_list
        self.font = font
        self.x = x
        self.y = y
        self.dx = dx
        self.dy = dy
        self.inst_width = w
        self.inst_height = h
        self.angle = angle

    def render(self, surf, state): 
        x = self.x 
        y = self.y
        idx = 0
        for inst in self.inst_list:
            inst_str = str(idx) + '. ' + inst
            if idx >= 0 and idx < 10: 
                inst_str = '0' + inst_str
            inst_surf = self.font.render(inst_str, True, (255,255,255))
            inst_surf = pygame.transform.rotate(inst_surf, self.angle)
            inst_rect = inst_surf.get_rect()
            inst_rect.bottomleft = (x,y)
            surf.blit(inst_surf, inst_rect)
            x += self.dx
            y += self.dy
            idx += 1
        pygame.draw.circle(surf, (250,50,0), 
                (self.x + self.dx * (state.heads['IP'] + 0.5), self.y + 5), 5)

class OrgState:
    def __init__(self, idx, heads, regs):
        self.idx = idx
        self.heads = heads
        self.regs = regs

if __name__ == '__main__':
    pygame.init()
    is_fullscreen = False
    tracer = AvidaTracer(1920, 1080, is_fullscreen, 'arial', 24)
    filename = '../settings/VirtualCPUOrg/example_patch_harvest.org'
    inst_font = pygame.font.SysFont('arial', 13)
    tracer.set_genome(filename, inst_font, 0, 200, 14, 0, 200, 14, 90)
    #tracer.parse_mabe_trace('test_run.txt')
    #tracer.parse_mabe_trace('test_run_2.txt')
    tracer.parse_avida_trace('/media/austin/samsung_nvme_pop/dropbox/msu/Dropbox/Mat_Mining_Experiment/Sample_Results/Rectangular_w_Edges_Environment/grid_4/org-2572919.trace')
    
    tracer_2 = AvidaTracer(1920, 1080, is_fullscreen, 'arial', 24)
    tracer_2.set_genome(filename, inst_font, 0, 200, 14, 0, 200, 14, 90)
    tracer_2.parse_mabe_trace('test_run.txt')

    tracer.compare_org_trace(tracer_2, 0)
    tracer.run()
