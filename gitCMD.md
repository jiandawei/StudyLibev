# Git 常用命令大全 - 远程仓库操作指南

本文档包含直接基于 main 分支合并到远端仓库、创建 PR、同步远端仓库代码到本地的相关命令。

## 📚 目录

- [直接推送到远端 main 分支](#直接推送到远端-main-分支)
- [创建 Pull Request (PR)](#创建-pull-request-pr)
- [同步远端代码到本地](#同步远端代码到本地)
- [分支管理](#分支管理)
- [故障排查](#故障排查)

---

## 🚀 直接推送到远端 main 分支

### 基础推送命令

```bash
# 直接推送到远端 main 分支
git push -u origin main

# 推送当前分支到对应的远端分支
git push

# 推送并设置上游分支
git push -u origin <branch-name>
```

### 强制推送（谨慎使用）

```bash
# 强制推送远端分支（覆盖远程）
git push --force origin main

# 强制推送（安全模式，只有当本地是远程的后代时才允许）
git push --force-with-lease origin main
```

### 推送多个分支

```bash
# 推送所有分支
git push --all origin

# 推送所有标签
git push --tags origin

# 推送特定标签
git push origin <tag-name>
```

### 推送场景示例

```bash
# 场景 1: 直接推送到 main 分支
git add .
git commit -m "直接更新的描述"
git push -u origin main

# 场景 2: 推送到临时分支
git checkout -b temp-branch
git add .
git commit -m "临时修改"
git push -u origin temp-branch

# 场景 3: 推送并覆盖远端（紧急情况）
git add .
git commit -m "紧急修复"
git push --force-with-lease origin main
```

---

## 🔄 创建 Pull Request (PR)

### 基础 PR 创建流程

```bash
# 1. 创建新分支
git checkout -b feature/your-feature-name

# 2. 进行修改
# ... 编辑文件 ...

# 3. 提交更改
git add .
git commit -m "描述您的更改"

# 4. 推送分支
git push -u origin feature/your-feature-name

# 5. 创建 PR（命令行）
# 使用 GitHub CLI (需要安装 gh)
gh pr create --title "PR 标题" --body "PR 描述"

# 或者在浏览器中打开
gh pr create --web
```

### PR 创建完整示例

```bash
# 完整的 PR 创建流程
git checkout -b feature/add-new-functionality
git add src/new_function.c
git commit -m "Add new functionality for feature X"
git push -u origin feature/add-new-functionality

# 创建 PR 并指定标题和描述
gh pr create \
  --title "Add new functionality for feature X" \
  --body "## 变更说明
添加了新功能以支持特性 X。

## 测试
- [ ] 单元测试通过
- [ ] 集成测试通过

## 文档
- [ ] 已更新相关文档"
```

### 高级 PR 操作命令

```bash
# 查看当前分支的 PR 状态
gh pr view

# 查看所有 PR
gh pr list

# 合并 PR
gh pr merge <pr-number>

# 关闭 PR
gh pr close <pr-number>

# 更新 PR 描述
gh pr edit <pr-number> --body "新的描述"
```

### PR 分支命名约定

```bash
# 功能开发
git checkout -b feature/user-authentication
git checkout -b feature/payment-integration

# 错误修复
git checkout -b bugfix/login-error
git checkout -b bugfix/memory-leak

# 紧急修复
git checkout -b hotfix/critical-security-fix

# 文档更新
git checkout -b docs/api-documentation
git checkout -b docs/installation-guide

# 重构
git checkout -b refactor/code-optimization
git checkout -b refactor/clean-architecture

# 测试
git checkout -b test/unit-tests-improvement
git checkout -b test/integration-tests
```

### PR 创建后的常用命令

```bash
# 更新 PR（在分支上有新提交）
git push origin feature/your-feature-name  # PR 会自动更新

# 查看 PR 讨论和状态
gh pr view --comments

# 请求代码审查
gh pr edit --add-reviewer username1,username2

# 查看 PR 的分支状态
git log origin/main..origin/feature/your-feature-name

# 本地测试 PR
gh pr checkout <pr-number>
```

---

## 📥 同步远端代码到本地

### 基础同步命令

```bash
# 获取并合并远端更新（最常用）
git pull

# 获取并合并指定分支
git pull origin main

# 获取但不合并
git fetch

# 获取并使用 rebase 合并
git pull --rebase

# 获取指定远程仓库
git fetch origin
```

### 同步远端详细操作

```bash
# 完整的同步流程
git fetch origin                    # 获取远端更新
git log --oneline origin/main       # 查看远端最新提交
git diff origin/main                # 查看具体差异
git pull origin main                # 合并远端更新

# 或者使用 rebase（保持历史干净）
git fetch origin
git rebase origin/main
```

### 查看远端状态

```bash
# 查看当前分支的远端追踪信息
git branch -vv

# 查看所有远端分支
git branch -r

# 查看本地和远端的差异
git diff HEAD origin/main

# 查看本地落后的提交
git log main..origin/main

# 查看本地领先远端的提交
git log origin/main..main

# 查看远端仓库详细信息
git remote show origin
```

### 处理同步冲突

```bash
# 场景 1: 简单的 git pull 遇到冲突
git pull origin main
# 解决冲突文件
git add 解决冲突的文件
git commit -m "Merge remote-tracking branch 'origin/main'"

# 场景 2: 使用 rebase 遇到冲突
git pull --rebase origin main
# 解决冲突文件
git add 解决冲突的文件
git rebase --continue

# 场景 3: 有未提交的本地修改，想要同步
git stash push -m "临时保存"        # 保存本地修改
git pull origin main                # 同步远端
git stash pop                       # 恢复本地修改

# 场景 4: 放弃本地修改，完全使用远端代码
git reset --hard origin/main
```

### 高级同步技巧

```bash
# 同步所有远端更新
git fetch --all

# 清理已删除的远端分支引用
git fetch --prune
git remote prune origin

# 同步特定远端分支到本地
git fetch origin feature-branch

# 合并远端分支到当前分支
git merge origin/feature-branch

# 变基远端分支
git rebase origin/feature-branch

# 查看所有分支关系
git log --oneline --graph --all --decorate
```

### 不同分支的同步策略

```bash
# 主分支同步
git checkout main
git fetch origin
git pull origin main

# 功能分支同步（保持基于最新 main）
git checkout feature/your-feature
git fetch origin
git rebase origin/main

# 功能分支同步（合并最新 main）
git checkout feature/your-feature
git fetch origin
git merge origin/main

# 多个远端同步
git fetch --all
git pull origin main
git pull upstream main
```

---

## 🌳 分支管理

### 本地分支操作

```bash
# 查看本地分支
git branch

# 查看所有分支（包括远端）
git branch -a

# 查看所有分支的追踪关系
git branch -vv

# 创建新分支
git branch new-branch

# 创建并切换到新分支
git checkout -b new-branch

# 切换分支
git checkout existing-branch

# 删除本地分支
git branch -d feature-branch

# 强制删除本地分支
git branch -D feature-branch

# 重命名分支
git branch -m old-name new-name
```

### 远端分支操作

```bash
# 推送新分支到远端
git push -u origin new-branch

# 从远端分支创建本地分支
git checkout -b local-branch origin/remote-branch

# 删除远端分支
git push origin --delete feature-branch

# 删除本地追踪的远端分支引用
git branch -r -d origin/feature-branch
```

### 分支对比和合并

```bash
# 比较两个分支
git diff branch1 branch2

# 查看分支1有但分支2没有的提交
git log branch2..branch1

# 合并分支
git merge feature-branch

# 变基分支
git rebase feature-branch

# 查看合并图
git log --oneline --graph
git log --oneline --graph --all
```

---

## 🔧 故障排查

### 常见问题解决方案

```bash
# 问题 1: 推送被拒绝 - 本地不是最新的
# 解决方案：
git pull origin main
git push origin main

# 问题 2: 推送被拒绝 - 远端有推送而本地有未拉取的更新
# 解决方案：
git fetch origin
git rebase origin/main
git push origin main

# 问题 3: 误删了远端分支
# 解决方案：
git reflog                    # 查找之前的提交
git checkout -b restored-branch <commit-hash>
git push -u origin restored-branch

# 问题 4: 误合并了错误的分支
# 解决方案：
git reset --hard HEAD~1       # 撤销最后一次合并
# 使用 reflog 找到正确的提交
git reflog
git reset --hard <correct-commit>

# 问题 5: 无法连接到远端仓库
# 解决方案：
git remote -v                  # 检查远程配置
git remote set-url origin git@github.com:user/repo.git
ssh -T git@github.com          # 测试 SSH 连接
```

### 状态检查命令

```bash
# 检查当前状态
git status

# 查看提交历史
git log --oneline

# 查看修改历史
git log --oneline --all

# 查看谁修改了什么
git blame file.txt

# 查看文件修改统计
git log --stat

# 查看分支模型图
git log --oneline --graph --all --decorate
```

### 网络连接问题

```bash
# 测试连接
ping github.com
ssh -T git@github.com

# 检查网络延迟
time git fetch origin

# 设置更长的超时时间
git config --global http.timeout 120
git config --global http.postBuffer 524288000

# 查看详细的调试信息
GIT_CURL_VERBOSE=1 git fetch origin

# 使用代理（如果需要）
git config --global http.proxy http://proxy.address:port
git config --global https.proxy http://proxy.address:port
```

---

## 🎯 工作流程最佳实践

### 日常开发流程

```bash
# 开始新功能开发
git checkout main
git pull origin main
git checkout -b feature/awesome-feature
# ... 开发 ...
git add .
git commit -m "Add awesome feature"
git push -u origin feature/awesome-feature
# 创建 PR
gh pr create --web

# 日常同步
git fetch origin
git rebase origin/main
git push --force-with-lease origin feature/your-feature
```

### 紧急修复流程

```bash
# 快速修复紧急问题
git checkout main
git pull origin main
git checkout -b hotfix/critical-bug
# ... 修复 ...
git add .
git commit -m "Fix critical bug"
git push -u origin hotfix/critical-bug
# 立即合并到 main
git checkout main
git merge hotfix/critical-bug
git push origin main
```

### 版本发布流程

```bash
# 创建发布分支
git checkout main
git checkout -b release/v1.0.0
# ... 最后的修复和测试 ...
git push -u origin release/v1.0.0
# 创建 PR 到 main，合并后打标签
git tag v1.0.0
git push origin v1.0.0
```

---

## 📝 常用 Git 配置

```bash
# 基础配置
git config --global user.name "Your Name"
git config --global user.email "your.email@example.com"

# 设置默认编辑器
git config --global core.editor "vim"
git config --global core.editor "nano"

# 设置默认分支名
git config --global init.defaultBranch main

# 设置默认 pull 使用 rebase
git config --global pull.rebase true

# 设置更安全的别名
git config --global alias.pu 'pull --rebase'
git config --global alias.st 'status'
git config --global alias.co 'checkout'
git config --global alias.br 'branch'

# 设置文件结束符处理（Windows）
git config --global core.autocrlf true
# 设置文件结束符处理（Unix）
git config --global core.autocrlf input
```

---

## 🔗 相关资源

- [Git 官方文档](https://git-scm.com/doc)
- [GitHub 文档](https://docs.github.com)
- [Git 命令速查表](https://education.github.com/git-cheat-sheet-education.pdf)
- [Git Flow 工作流](https://nvie.com/posts/a-successful-git-branching-model/)

---

**注意**: 使用强制推送（`push --force`）时要非常小心，可能会导致远端仓库的数据丢失。推荐使用 `push --force-with-lease` 来作为安全替代。